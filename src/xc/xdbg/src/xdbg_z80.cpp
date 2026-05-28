#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <xz80/cpu.hpp>
#include <xz80/cpu_state.hpp>
#include <xz80/memory.hpp>
#include <xz80/ports.hpp>

#include <xdbgstub/xdbgstub.hpp>

namespace {

    struct options {
        std::string listen_host = "127.0.0.1";
        uint16_t listen_port = 9000;
        std::optional<std::filesystem::path> binary_path;
        uint16_t origin = 0x0000;
        uint16_t pc = 0x0000;
        uint16_t sp = 0xFFFF;
        bool quiet = false;
        bool show_help = false;
    };

    class z80_target final : public xdbgstub::target {
    public:
        z80_target()
            : cpu_(mem_, ports_)
        {
            cpu_.reset();
            cpu_.set_reg(xz80::reg16::SP, 0xFFFF);
        }

        void load_binary(const std::filesystem::path& path, uint16_t origin) {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("cannot open binary: " + path.string());
            }

            const auto bytes = std::vector<uint8_t>(
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                mem_.write(
                    static_cast<uint16_t>((origin + static_cast<uint16_t>(i)) & 0xFFFF),
                    bytes[i]);
            }
        }

        void set_initial_pc(uint16_t pc) {
            cpu_.set_reg(xz80::reg16::PC, pc);
        }

        void set_initial_sp(uint16_t sp) {
            cpu_.set_reg(xz80::reg16::SP, sp);
        }

        xdbgstub::target_status status() override {
            xdbgstub::target_status s;
            s.state = running_ ? xdbgstub::execution_state::running
                                : xdbgstub::execution_state::stopped;
            if (terminated_) {
                s.state = xdbgstub::execution_state::terminated;
            }
            s.reason = last_reason_;
            s.pc = cpu_.pc();
            if (exit_code_.has_value()) {
                s.exit_code = exit_code_;
            }
            s.registers = capture_cpu_state();
            return s;
        }

        xdbgstub::cpu_state read_registers() override {
            return capture_cpu_state();
        }

        void write_registers(const xdbgstub::cpu_state& state) override {
            apply_cpu_state(state);
        }

        std::vector<uint8_t> read_memory(
            uint32_t address, std::size_t length) override
        {
            std::vector<uint8_t> data;
            data.reserve(length);
            for (std::size_t i = 0; i < length; ++i) {
                data.push_back(
                    mem_.read(static_cast<uint16_t>((address + i) & 0xFFFF)));
            }
            return data;
        }

        void write_memory(uint32_t address, const std::vector<uint8_t>& data) override {
            for (std::size_t i = 0; i < data.size(); ++i) {
                mem_.write(
                    static_cast<uint16_t>((address + i) & 0xFFFF),
                    data[i]);
            }
        }

        xdbgstub::target_status continue_execution() override {
            if (terminated_) {
                return status();
            }
            running_ = true;
            constexpr std::size_t max_instructions = 1000000;
            for (std::size_t i = 0; i < max_instructions; ++i) {
                const uint16_t pc = cpu_.pc();
                if (has_breakpoint(pc)) {
                    running_ = false;
                    last_reason_ = xdbgstub::stop_reason::breakpoint;
                    return status();
                }

                cpu_.step();
                if (cpu_.halted()) {
                    running_ = false;
                    terminated_ = true;
                    exit_code_ = cpu_.reg(xz80::reg16::DE);
                    last_reason_ = xdbgstub::stop_reason::exited;
                    return status();
                }
            }

            running_ = false;
            last_reason_ = xdbgstub::stop_reason::pause;
            return status();
        }

        xdbgstub::target_status step_instruction() override {
            if (terminated_) {
                return status();
            }
            running_ = false;
            cpu_.step();
            if (cpu_.halted()) {
                terminated_ = true;
                exit_code_ = cpu_.reg(xz80::reg16::DE);
                last_reason_ = xdbgstub::stop_reason::exited;
            } else {
                last_reason_ = xdbgstub::stop_reason::step;
            }
            return status();
        }

        xdbgstub::target_status pause_execution() override {
            running_ = false;
            last_reason_ = xdbgstub::stop_reason::pause;
            return status();
        }

        void set_breakpoint(uint32_t address) override {
            if (!has_breakpoint(static_cast<uint16_t>(address))) {
                breakpoints_.push_back(static_cast<uint16_t>(address));
            }
        }

        void clear_breakpoint(uint32_t address) override {
            const uint16_t addr16 = static_cast<uint16_t>(address);
            breakpoints_.erase(
                std::remove(breakpoints_.begin(), breakpoints_.end(), addr16),
                breakpoints_.end());
        }

        void detach() override {
            running_ = false;
        }

    private:
        bool has_breakpoint(uint16_t address) const {
            return std::find(breakpoints_.begin(), breakpoints_.end(), address)
                != breakpoints_.end();
        }

        xdbgstub::cpu_state capture_cpu_state() const {
            const xz80::cpu_state snap = cpu_.snapshot();
            xdbgstub::cpu_state state;
            state.af     = snap.af;
            state.bc     = snap.bc;
            state.de     = snap.de;
            state.hl     = snap.hl;
            state.ix     = snap.ix;
            state.iy     = snap.iy;
            state.sp     = snap.sp;
            state.pc     = snap.pc;
            state.i      = snap.i;
            state.r      = snap.r;
            state.iff1   = snap.iff1;
            state.iff2   = snap.iff2;
            state.halted = snap.halted;
            return state;
        }

        void apply_cpu_state(const xdbgstub::cpu_state& state) {
            xz80::cpu_state snap = cpu_.snapshot(); // preserve shadow regs and im
            snap.af     = state.af;
            snap.bc     = state.bc;
            snap.de     = state.de;
            snap.hl     = state.hl;
            snap.ix     = state.ix;
            snap.iy     = state.iy;
            snap.sp     = state.sp;
            snap.pc     = state.pc;
            snap.i      = state.i;
            snap.r      = state.r;
            snap.iff1   = state.iff1;
            snap.iff2   = state.iff2;
            snap.halted = state.halted;
            cpu_.restore(snap);
        }

        xz80::flat_memory      mem_;
        xz80::null_ports       ports_;
        xz80::cpu              cpu_;
        std::vector<uint16_t>  breakpoints_;
        xdbgstub::stop_reason  last_reason_ = xdbgstub::stop_reason::none;
        bool                   running_     = false;
        bool                   terminated_  = false;
        std::optional<uint32_t> exit_code_;
    };

    void print_help() {
        std::cout
            << "xdbg-z80 - simple z80 remote debug target\n"
            << "usage: xdbg-z80 [options]\n\n"
            << "options:\n"
            << "  --listen HOST:PORT   listen address (default 127.0.0.1:9000)\n"
            << "  --load-bin FILE      load raw binary into memory\n"
            << "  --origin ADDR        binary load origin (default 0x0000)\n"
            << "  --pc ADDR            initial PC (default origin)\n"
            << "  --sp ADDR            initial SP (default 0xFFFF)\n"
            << "  -q, --quiet          quiet startup\n"
            << "  -h, --help           show this help\n";
    }

    uint32_t parse_u32(const std::string& value) {
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
        if (end == value.c_str() || *end != '\0') {
            throw std::runtime_error("invalid number: " + value);
        }
        return static_cast<uint32_t>(parsed);
    }

    std::pair<std::string, uint16_t> split_host_port(const std::string& value) {
        const auto colon = value.rfind(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("expected host:port");
        }
        return {
            value.substr(0, colon),
            static_cast<uint16_t>(parse_u32(value.substr(colon + 1)))
        };
    }

    options parse_options(int argc, char* argv[]) {
        options opts;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            } else if (arg == "-q" || arg == "--quiet") {
                opts.quiet = true;
            } else if (arg == "--listen") {
                if (++i >= argc) {
                    throw std::runtime_error("--listen requires host:port");
                }
                const auto [host, port] = split_host_port(argv[i]);
                opts.listen_host = host;
                opts.listen_port = port;
            } else if (arg == "--load-bin") {
                if (++i >= argc) {
                    throw std::runtime_error("--load-bin requires a path");
                }
                opts.binary_path = argv[i];
            } else if (arg == "--origin") {
                if (++i >= argc) {
                    throw std::runtime_error("--origin requires a value");
                }
                opts.origin = static_cast<uint16_t>(parse_u32(argv[i]));
            } else if (arg == "--pc") {
                if (++i >= argc) {
                    throw std::runtime_error("--pc requires a value");
                }
                opts.pc = static_cast<uint16_t>(parse_u32(argv[i]));
            } else if (arg == "--sp") {
                if (++i >= argc) {
                    throw std::runtime_error("--sp requires a value");
                }
                opts.sp = static_cast<uint16_t>(parse_u32(argv[i]));
            } else {
                throw std::runtime_error("unknown option: " + arg);
            }
        }
        return opts;
    }

} // namespace

int main(int argc, char* argv[]) {
    try {
        auto opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help();
            return 0;
        }

        z80_target target;
        if (opts.binary_path.has_value()) {
            target.load_binary(opts.binary_path.value(), opts.origin);
        }
        target.set_initial_pc(opts.pc == 0x0000 && opts.binary_path.has_value()
            ? opts.origin
            : opts.pc);
        target.set_initial_sp(opts.sp);

        xdbgstub::server server;
        server.listen(opts.listen_host, opts.listen_port);

        if (!opts.quiet) {
            std::cout << "xdbg-z80 listening on "
                      << opts.listen_host << ":" << opts.listen_port << "\n";
            if (opts.binary_path.has_value()) {
                std::cout << "loaded " << opts.binary_path.value()
                          << " at 0x" << std::hex << opts.origin << std::dec << "\n";
            }
        }

        while (server.is_listening()) {
            try {
                server.serve(target);
            } catch (const xdbgstub::error& e) {
                if (!server.is_listening()) {
                    break;
                }
                if (std::string(e.what()) == "connection closed") {
                    if (!opts.quiet) {
                        std::cout << "xdbg-z80 client disconnected, waiting for reconnect\n";
                    }
                    continue;
                }
                throw;
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "xdbg-z80: " << e.what() << "\n";
        return 1;
    }
}
