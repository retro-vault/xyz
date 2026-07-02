#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

#include "debugger_session.h"
#include "frontends.h"
#include <xgdb/debug_protocol.h>

namespace {

#ifndef XTOOLS_VERSION
#define XTOOLS_VERSION "0.1.0"
#endif

    enum class frontend_mode {
        cli,
        mi
    };

    struct options {
        frontend_mode mode = frontend_mode::cli;
        std::optional<std::filesystem::path> exec_file;
        std::optional<std::filesystem::path> cdb_file;
        std::optional<std::filesystem::path> map_file;
        std::optional<std::string> remote_target;
        std::vector<std::string> execute_commands;
        std::vector<std::filesystem::path> source_dirs;
        std::optional<std::filesystem::path> log_file;
        bool show_help = false;
        bool show_version = false;
    };

    void configure_terminal_input() {
#ifndef _WIN32
        // When stdin is a terminal (e.g. a PTY under DDD), disable echo so the
        // front-end's own output is not echoed back to us as commands.
        if (isatty(STDIN_FILENO)) {
            struct termios t;
            if (tcgetattr(STDIN_FILENO, &t) == 0) {
                t.c_lflag &= ~static_cast<tcflag_t>(ECHO | ECHOE | ECHOK | ECHONL);
                tcsetattr(STDIN_FILENO, TCSANOW, &t);
            }
        }
#endif
    }

    void print_help() {
        std::cout
            << "Usage: xgdb [options] [program]\n\n"
            << "X Tools Debugger (xgdb) — remote Z80 debugger\n\n"
            << "startup:\n"
            << "  --exec <file>           target binary image\n"
            << "  --cdb <file>            SDCC CDB debug information file\n"
            << "  --map <file>            SDCC MAP linker output file (optional)\n"
            << "  --remote <host:port>    connect to remote target\n"
            << "  -d <dir>                add source search directory\n"
            << "  --directory <dir>       add source search directory\n"
            << "  --log <file>            log all protocol I/O to file\n"
            << "  --interpreter <mode>    frontend mode: cli, mi, or mi2\n"
            << "  --interpreter=<mode>    frontend mode: cli, mi, or mi2\n"
            << "  --mi                    shorthand for --interpreter=mi\n"
            << "  -ex <command>           execute debugger command\n"
            << "\n"
            << "compatibility:\n"
            << "  -q, --quiet             accepted for GDB frontend compatibility (ignored)\n"
            << "  --nx, -nx               accepted for GDB frontend compatibility (ignored)\n"
            << "  --fullname, -fullname   accepted for DDD compatibility (ignored)\n"
            << "  --tty <path>            accepted for DDD compatibility (ignored)\n"
            << "  --tty=<path>            accepted for DDD compatibility (ignored)\n"
            << "\n"
            << "  --version               print version\n"
            << "  -h, --help              show this help\n";
    }

    options parse_options(int argc, char* argv[]) {
        options opts;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            } else if (arg == "--version") {
                opts.show_version = true;
                return opts;
            } else if (arg == "--mi") {
                opts.mode = frontend_mode::mi;
            } else if (arg == "--interpreter") {
                if (++i >= argc) {
                    throw std::runtime_error("--interpreter requires a value");
                }
                if (std::string(argv[i]) == "mi" || std::string(argv[i]) == "mi2") {
                    opts.mode = frontend_mode::mi;
                } else if (std::string(argv[i]) == "cli") {
                    opts.mode = frontend_mode::cli;
                } else {
                    throw std::runtime_error("unsupported interpreter mode");
                }
            } else if (arg.rfind("--interpreter=", 0) == 0) {
                const auto mode = arg.substr(std::string("--interpreter=").size());
                if (mode == "mi" || mode == "mi2") {
                    opts.mode = frontend_mode::mi;
                } else if (mode == "cli") {
                    opts.mode = frontend_mode::cli;
                } else {
                    throw std::runtime_error("unsupported interpreter mode");
                }
            } else if (arg == "--exec") {
                if (++i >= argc) {
                    throw std::runtime_error("--exec requires a path");
                }
                opts.exec_file = argv[i];
            } else if (arg == "--cdb") {
                if (++i >= argc) {
                    throw std::runtime_error("--cdb requires a path");
                }
                opts.cdb_file = argv[i];
            } else if (arg == "--map") {
                if (++i >= argc) {
                    throw std::runtime_error("--map requires a path");
                }
                opts.map_file = argv[i];
            } else if (arg == "--remote") {
                if (++i >= argc) {
                    throw std::runtime_error("--remote requires host:port");
                }
                opts.remote_target = argv[i];
            } else if (arg == "--log") {
                if (++i >= argc) {
                    throw std::runtime_error("--log requires a file path");
                }
                opts.log_file = argv[i];
            } else if (arg == "-d" || arg == "--directory") {
                if (++i >= argc) {
                    throw std::runtime_error("-d requires a directory");
                }
                opts.source_dirs.push_back(argv[i]);
            } else if (arg == "-ex") {
                if (++i >= argc) {
                    throw std::runtime_error("-ex requires a command");
                }
                opts.execute_commands.push_back(argv[i]);
            } else if (arg == "--nx" || arg == "-nx"
                       || arg == "--fullname" || arg == "-fullname"
                       || arg == "-q" || arg == "--quiet") {
                // Flags GDB front-ends (DDD etc.) may append; accept silently.
                continue;
            } else if (arg == "--tty") {
                if (++i >= argc) {
                    throw std::runtime_error("--tty requires a path");
                }
                continue;
            } else if (arg.rfind("--tty=", 0) == 0) {
                continue;
            } else if (!arg.empty() && arg[0] == '-') {
                throw std::runtime_error("unknown option: " + arg);
            } else if (!opts.exec_file.has_value()) {
                opts.exec_file = arg;
            } else {
                throw std::runtime_error("unexpected argument: " + arg);
            }
        }
        return opts;
    }

} // namespace

int main(int argc, char* argv[]) {
    configure_terminal_input();

    try {
        auto opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help();
            return 0;
        }
        if (opts.show_version) {
            std::cout << "xgdb " << XTOOLS_VERSION
                      << " (X Tools Debugger for Z80)\n";
            return 0;
        }

        // Open log file and dump argv so we can see exactly what DDD passed.
        std::ofstream log_stream;
        std::unique_ptr<tee_streambuf> tee_out;
        std::unique_ptr<tee_streambuf> tee_err;
        std::streambuf* orig_cout = nullptr;
        std::streambuf* orig_cerr = nullptr;

        if (opts.log_file.has_value()) {
            log_stream.open(opts.log_file.value(), std::ios::out | std::ios::trunc);
            if (log_stream.is_open()) {
                log_stream << "[ARGV]";
                for (int i = 0; i < argc; ++i)
                    log_stream << " " << argv[i];
                log_stream << "\n" << std::flush;

                // Tee stdout and stderr through the log file.
                orig_cout = std::cout.rdbuf();
                orig_cerr = std::cerr.rdbuf();
                tee_out = std::make_unique<tee_streambuf>(orig_cout, log_stream.rdbuf());
                tee_err = std::make_unique<tee_streambuf>(orig_cerr, log_stream.rdbuf());
                std::cout.rdbuf(tee_out.get());
                std::cerr.rdbuf(tee_err.get());
            }
        }

        debugger_session session;
        if (opts.exec_file.has_value()) {
            session.set_exec_path(opts.exec_file.value());
        }
        if (opts.cdb_file.has_value()) {
            session.load_cdb_file(opts.cdb_file.value());
        } else {
            session.maybe_load_default_symbols();
        }
        if (opts.map_file.has_value()) {
            session.load_map_file(opts.map_file.value());
        }
        for (const auto& dir : opts.source_dirs) {
            session.add_source_dir(dir);
        }

        // Defer remote connection until after the first prompt is emitted.
        // Blocking on TCP connect before the prompt causes DDD and other
        // front-ends to miss the startup prompt and hang.
        if (opts.remote_target.has_value()) {
            const std::string cmd = "target remote " + opts.remote_target.value();
            opts.execute_commands.insert(opts.execute_commands.begin(), cmd);
        }

        std::unique_ptr<debug_protocol> ui;
        if (opts.mode == frontend_mode::mi) {
            auto mi = std::make_unique<mi_frontend>(session);
            if (log_stream.is_open()) mi->set_log(&log_stream);
            ui = std::move(mi);
        } else {
            auto cli = std::make_unique<cli_frontend>(
                session, std::cin, std::cout, std::cerr, true);
            cli->set_execute_commands(opts.execute_commands);
            if (log_stream.is_open()) cli->set_log(&log_stream);
            ui = std::move(cli);
        }

        const int rc = ui->run();

        // Restore original stdout/stderr before log_stream closes.
        if (orig_cout) std::cout.rdbuf(orig_cout);
        if (orig_cerr) std::cerr.rdbuf(orig_cerr);
        return rc;
    } catch (const std::exception& e) {
        std::cerr << "xgdb: " << e.what() << "\n";
        return 1;
    }
}
