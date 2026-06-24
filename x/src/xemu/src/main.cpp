#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <rsp/rsp.h>
#include <xemu/xemu.h>

namespace {

struct options {
    std::string listen_host = "127.0.0.1";
    uint16_t listen_port = 9000;
    std::optional<std::filesystem::path> binary_path;
    uint16_t origin = 0x0000;
    std::optional<uint16_t> pc;
    uint16_t sp = 0xFFFF;
    std::optional<uint16_t> stdin_port;
    std::optional<uint16_t> stdout_port;
    std::size_t max_steps = 1'000'000;
    bool quiet = false;
    bool run_mode = false;
    bool show_help = false;
};

void print_help() {
    std::cout
        << "xemu - Z80 emulator\n"
        << "usage: xemu [options]\n\n"
        << "modes:\n"
        << "  default              start an RSP server for xgdb-compatible clients\n"
        << "  --run                execute immediately until HALT or the step limit\n\n"
        << "options:\n"
        << "  --listen HOST:PORT   listen address (default 127.0.0.1:9000)\n"
        << "  --run                run immediately instead of waiting for a debugger\n"
        << "  --max-steps N        step budget for --run (default 1000000)\n"
        << "  --load-bin FILE      load raw binary into memory\n"
        << "  --origin ADDR        binary load address (default 0x0000)\n"
        << "  --pc ADDR            initial program counter (default: origin)\n"
        << "  --sp ADDR            initial stack pointer (default 0xFFFF)\n"
        << "  --stdin-port ADDR    map Z80 port ADDR to host stdin\n"
        << "  --stdout-port ADDR   map Z80 port ADDR to host stdout\n"
        << "  -q, --quiet          quiet startup\n"
        << "  -h, --help           show this help\n";
}

uint32_t parse_u32(const std::string& value) {
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0')
        throw std::runtime_error("invalid number: " + value);
    return static_cast<uint32_t>(parsed);
}

std::pair<std::string, uint16_t> split_host_port(const std::string& value) {
    const auto colon = value.rfind(':');
    if (colon == std::string::npos)
        throw std::runtime_error("expected host:port");
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
            if (++i >= argc) throw std::runtime_error("--listen requires host:port");
            const auto [host, port] = split_host_port(argv[i]);
            opts.listen_host = host;
            opts.listen_port = port;
        } else if (arg == "--run") {
            opts.run_mode = true;
        } else if (arg == "--max-steps") {
            if (++i >= argc) throw std::runtime_error("--max-steps requires a value");
            opts.max_steps = static_cast<std::size_t>(parse_u32(argv[i]));
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
        } else if (arg == "--stdin-port") {
            if (++i >= argc) throw std::runtime_error("--stdin-port requires a value");
            opts.stdin_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--stdout-port") {
            if (++i >= argc) throw std::runtime_error("--stdout-port requires a value");
            opts.stdout_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }
    return opts;
}

void configure_machine(xemu::machine& emu, const options& opts) {
    if (opts.binary_path.has_value())
        emu.load_binary(opts.binary_path.value(), opts.origin);
    if (opts.stdin_port.has_value())
        emu.bind_stdin(opts.stdin_port.value(), std::cin);
    if (opts.stdout_port.has_value())
        emu.bind_stdout(opts.stdout_port.value(), std::cout);

    emu.set_pc(opts.pc.has_value() ? opts.pc.value() : opts.origin);
    emu.set_sp(opts.sp);
}

int run_program(xemu::machine& emu, const options& opts) {
    const auto stop = emu.continue_execution(opts.max_steps);
    if (!opts.quiet) {
        switch (stop.reason) {
        case xemu::stop_reason::halted:
            std::cerr << "xemu: halted at 0x" << std::hex << stop.pc << std::dec << "\n";
            break;
        case xemu::stop_reason::breakpoint:
            std::cerr << "xemu: breakpoint at 0x" << std::hex << stop.pc << std::dec << "\n";
            break;
        case xemu::stop_reason::step_limit:
            std::cerr << "xemu: step limit reached at 0x" << std::hex << stop.pc << std::dec << "\n";
            break;
        case xemu::stop_reason::fault:
            std::cerr << "xemu: fault at 0x" << std::hex << stop.pc << std::dec;
            if (!stop.message.empty()) {
                std::cerr << " - " << stop.message;
            }
            std::cerr << "\n";
            break;
        case xemu::stop_reason::stepped:
        case xemu::stop_reason::none:
            break;
        }
    }

    return stop.reason == xemu::stop_reason::halted ? 0 : 2;
}

int serve_debugger(xemu::machine& emu, const options& opts) {
    xemu::rsp_target_adapter target(emu);
    rsp::server server;
    server.listen(opts.listen_host, opts.listen_port);

    if (!opts.quiet) {
        std::cout << "xemu listening on "
                  << opts.listen_host << ":" << opts.listen_port << "\n";
        if (opts.binary_path.has_value())
            std::cout << "loaded " << opts.binary_path.value()
                      << " at 0x" << std::hex << opts.origin << std::dec << "\n";
        if (opts.stdin_port.has_value())
            std::cout << "stdin mapped to port 0x" << std::hex
                      << opts.stdin_port.value() << std::dec << "\n";
        if (opts.stdout_port.has_value())
            std::cout << "stdout mapped to port 0x" << std::hex
                      << opts.stdout_port.value() << std::dec << "\n";
        std::cout << "connect with: target remote "
                  << opts.listen_host << ":" << opts.listen_port << "\n";
    }

    while (server.is_listening()) {
        try {
            server.serve(target);
            if (!opts.quiet)
                std::cout << "xemu client disconnected, waiting for reconnect\n";
        } catch (const rsp::error& e) {
            if (!server.is_listening()) break;
            if (std::string(e.what()).find("connection closed") != std::string::npos) {
                if (!opts.quiet)
                    std::cout << "xemu client disconnected\n";
                continue;
            }
            throw;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help();
            return 0;
        }

        xemu::machine emu;
        configure_machine(emu, opts);

        if (opts.run_mode)
            return run_program(emu, opts);
        return serve_debugger(emu, opts);
    } catch (const std::exception& e) {
        std::cerr << "xemu: " << e.what() << "\n";
        return 1;
    }
}
