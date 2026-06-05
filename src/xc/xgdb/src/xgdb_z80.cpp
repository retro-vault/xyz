#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <xz80/cpu.h>
#include <xz80/cpu_state.h>
#include <xz80/memory.h>
#include <xz80/ports.h>

#include <rsp/rsp.h>

namespace {

    struct options {
        std::string listen_host = "127.0.0.1";
        uint16_t    listen_port = 9000;
        std::optional<std::filesystem::path> binary_path;
        uint16_t origin = 0x0000;
        uint16_t pc     = 0x0000;
        uint16_t sp     = 0xFFFF;
        bool quiet      = false;
        bool show_help  = false;
    };

    // -----------------------------------------------------------------------
    // Z80 RSP target
    //
    // Register layout (18 bytes, little-endian 16-bit pairs):
    //   0:AF  2:BC  4:DE  6:HL  8:IX  10:IY  12:SP  14:PC  16:I  17:R
    // -----------------------------------------------------------------------

    class z80_target final : public rsp::target {
    public:
        z80_target() : cpu_(mem_, ports_) {
            cpu_.reset();
            cpu_.set_reg(xz80::reg16::SP, 0xFFFF);
        }

        void load_binary(const std::filesystem::path& path, uint16_t origin) {
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open())
                throw std::runtime_error("cannot open binary: " + path.string());
            std::vector<uint8_t> bytes{
                std::istreambuf_iterator<char>(f),
                std::istreambuf_iterator<char>()};
            for (std::size_t i = 0; i < bytes.size(); ++i)
                mem_.write(static_cast<uint16_t>((origin + i) & 0xFFFF), bytes[i]);
        }

        void set_pc(uint16_t pc) { cpu_.set_reg(xz80::reg16::PC, pc); }
        void set_sp(uint16_t sp) { cpu_.set_reg(xz80::reg16::SP, sp); }

        // --- rsp::target interface ---

        std::vector<uint8_t> read_registers() override {
            const xz80::cpu_state s = cpu_.snapshot();
            std::vector<uint8_t> d(18, 0);
            auto put16 = [&](std::size_t i, uint16_t v) {
                d[i]     = static_cast<uint8_t>(v & 0xFF);
                d[i + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
            };
            put16(0,  s.af);  put16(2,  s.bc);
            put16(4,  s.de);  put16(6,  s.hl);
            put16(8,  s.ix);  put16(10, s.iy);
            put16(12, s.sp);  put16(14, s.pc);
            d[16] = s.i;      d[17] = s.r;
            return d;
        }

        void write_registers(const std::vector<uint8_t>& d) override {
            if (d.size() < 18) return;
            xz80::cpu_state s = cpu_.snapshot();
            auto get16 = [&](std::size_t i) -> uint16_t {
                return static_cast<uint16_t>(d[i]) |
                       static_cast<uint16_t>(static_cast<uint16_t>(d[i + 1]) << 8);
            };
            s.af = get16(0);  s.bc = get16(2);
            s.de = get16(4);  s.hl = get16(6);
            s.ix = get16(8);  s.iy = get16(10);
            s.sp = get16(12); s.pc = get16(14);
            s.i  = d[16];     s.r  = d[17];
            cpu_.restore(s);
        }

        std::vector<uint8_t> read_memory(uint32_t addr, std::size_t len) override {
            std::vector<uint8_t> data;
            data.reserve(len);
            for (std::size_t i = 0; i < len; ++i)
                data.push_back(mem_.read(static_cast<uint16_t>((addr + i) & 0xFFFF)));
            return data;
        }

        void write_memory(uint32_t addr, const std::vector<uint8_t>& data) override {
            for (std::size_t i = 0; i < data.size(); ++i)
                mem_.write(static_cast<uint16_t>((addr + i) & 0xFFFF), data[i]);
        }

        std::string cont() override {
            if (terminated_) return last_stop_;
            constexpr std::size_t max_steps = 1'000'000;
            for (std::size_t i = 0; i < max_steps; ++i) {
                if (has_bp(cpu_.pc())) { last_stop_ = "S05"; return last_stop_; }
                cpu_.step();
                if (cpu_.halted()) { terminated_ = true; last_stop_ = "W00"; return last_stop_; }
            }
            last_stop_ = "S02";  // SIGINT — yielded after max steps
            return last_stop_;
        }

        std::string step() override {
            if (terminated_) return last_stop_;
            cpu_.step();
            if (cpu_.halted()) { terminated_ = true; last_stop_ = "W00"; }
            else                last_stop_ = "S05";
            return last_stop_;
        }

        std::string stop_reason() override { return last_stop_; }

        void insert_breakpoint(uint32_t addr) override {
            const uint16_t a = static_cast<uint16_t>(addr);
            if (!has_bp(a)) breakpoints_.push_back(a);
        }

        void remove_breakpoint(uint32_t addr) override {
            const uint16_t a = static_cast<uint16_t>(addr);
            breakpoints_.erase(
                std::remove(breakpoints_.begin(), breakpoints_.end(), a),
                breakpoints_.end());
        }

        void detach() override {}

    private:
        bool has_bp(uint16_t addr) const {
            return std::find(breakpoints_.begin(), breakpoints_.end(), addr)
                != breakpoints_.end();
        }

        xz80::flat_memory     mem_;
        xz80::null_ports      ports_;
        xz80::cpu             cpu_;
        std::vector<uint16_t> breakpoints_;
        std::string           last_stop_ = "S05";
        bool                  terminated_ = false;
    };

    void print_help() {
        std::cout
            << "xgdb-z80 - Z80 gdbserver\n"
            << "usage: xgdb-z80 [options]\n\n"
            << "  Speaks GDB Remote Serial Protocol over TCP.\n"
            << "  Any GDB-compatible debugger can connect to it.\n\n"
            << "options:\n"
            << "  --listen HOST:PORT   listen address (default 127.0.0.1:9000)\n"
            << "  --load-bin FILE      load raw binary into memory\n"
            << "  --origin ADDR        binary load address (default 0x0000)\n"
            << "  --pc ADDR            initial PC (default: origin)\n"
            << "  --sp ADDR            initial SP (default 0xFFFF)\n"
            << "  -q, --quiet          quiet startup\n"
            << "  -h, --help           show this help\n";
    }

    uint32_t parse_u32(const std::string& s) {
        char* end = nullptr;
        const unsigned long v = std::strtoul(s.c_str(), &end, 0);
        if (end == s.c_str() || *end != '\0')
            throw std::runtime_error("invalid number: " + s);
        return static_cast<uint32_t>(v);
    }

    std::pair<std::string, uint16_t> split_host_port(const std::string& s) {
        const auto colon = s.rfind(':');
        if (colon == std::string::npos) throw std::runtime_error("expected host:port");
        return {s.substr(0, colon), static_cast<uint16_t>(parse_u32(s.substr(colon + 1)))};
    }

    options parse_options(int argc, char* argv[]) {
        options opts;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") { opts.show_help = true; return opts; }
            else if (arg == "-q" || arg == "--quiet") opts.quiet = true;
            else if (arg == "--listen") {
                if (++i >= argc) throw std::runtime_error("--listen requires host:port");
                const auto [h, p] = split_host_port(argv[i]);
                opts.listen_host = h; opts.listen_port = p;
            } else if (arg == "--load-bin") {
                if (++i >= argc) throw std::runtime_error("--load-bin requires a path");
                opts.binary_path = argv[i];
            } else if (arg == "--origin") {
                if (++i >= argc) throw std::runtime_error("--origin requires a value");
                opts.origin = static_cast<uint16_t>(parse_u32(argv[i]));
            } else if (arg == "--pc") {
                if (++i >= argc) throw std::runtime_error("--pc requires a value");
                opts.pc = static_cast<uint16_t>(parse_u32(argv[i]));
            } else if (arg == "--sp") {
                if (++i >= argc) throw std::runtime_error("--sp requires a value");
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
        if (opts.show_help) { print_help(); return 0; }

        z80_target target;
        if (opts.binary_path.has_value())
            target.load_binary(opts.binary_path.value(), opts.origin);

        target.set_pc(opts.pc == 0x0000 && opts.binary_path.has_value()
                      ? opts.origin : opts.pc);
        target.set_sp(opts.sp);

        rsp::server server;
        server.listen(opts.listen_host, opts.listen_port);

        if (!opts.quiet) {
            std::cout << "xgdb-z80 (gdbserver) listening on "
                      << opts.listen_host << ":" << opts.listen_port << "\n";
            if (opts.binary_path.has_value())
                std::cout << "loaded " << opts.binary_path.value()
                          << " at 0x" << std::hex << opts.origin << std::dec << "\n";
            std::cout << "connect with: target remote "
                      << opts.listen_host << ":" << opts.listen_port << "\n";
        }

        while (server.is_listening()) {
            try {
                server.serve(target);
                if (!opts.quiet)
                    std::cout << "xgdb-z80 client disconnected, waiting for reconnect\n";
            } catch (const rsp::error& e) {
                if (!server.is_listening()) break;
                if (std::string(e.what()).find("connection closed") != std::string::npos) {
                    if (!opts.quiet)
                        std::cout << "xgdb-z80 client disconnected\n";
                    continue;
                }
                throw;
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "xgdb-z80: " << e.what() << "\n";
        return 1;
    }
}
