#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "debugger_session.hpp"
#include "frontends.hpp"
#include <xdbg/debug_protocol.hpp>

namespace {

    enum class frontend_mode {
        cli,
        mi,
        dap
    };

    struct options {
        frontend_mode mode = frontend_mode::cli;
        std::optional<std::filesystem::path> exec_file;
        std::optional<std::filesystem::path> symbols_file;
        std::optional<std::string> remote_target;
        std::vector<std::string> execute_commands;
        bool quiet = false;
        bool show_help = false;
    };

    void print_help() {
        std::cout
            << "xdbg - remote debugger for xdbg/xlink targets\n"
            << "usage: xdbg [options] [program]\n\n"
            << "options:\n"
            << "  --exec <file>           target binary image\n"
            << "  --symbols <file>        xdbg symbols database\n"
            << "  --remote <host:port>    connect to remote target\n"
            << "  --interpreter <mode>    frontend mode: cli, mi, or dap\n"
            << "  --interpreter=<mode>    frontend mode: cli, mi, or dap\n"
            << "  --mi                    shorthand for --interpreter=mi\n"
            << "  -ex <command>           execute debugger command\n"
            << "  -q, --quiet             quiet startup\n"
            << "  -h, --help              show this help\n";
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
            } else if (arg == "--mi") {
                opts.mode = frontend_mode::mi;
            } else if (arg == "--interpreter") {
                if (++i >= argc) {
                    throw std::runtime_error("--interpreter requires a value");
                }
                if (std::string(argv[i]) == "mi" || std::string(argv[i]) == "mi2") {
                    opts.mode = frontend_mode::mi;
                } else if (std::string(argv[i]) == "dap") {
                    opts.mode = frontend_mode::dap;
                } else if (std::string(argv[i]) == "cli") {
                    opts.mode = frontend_mode::cli;
                } else {
                    throw std::runtime_error("unsupported interpreter mode");
                }
            } else if (arg.rfind("--interpreter=", 0) == 0) {
                const auto mode = arg.substr(std::string("--interpreter=").size());
                if (mode == "mi" || mode == "mi2") {
                    opts.mode = frontend_mode::mi;
                } else if (mode == "dap") {
                    opts.mode = frontend_mode::dap;
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
            } else if (arg == "--symbols") {
                if (++i >= argc) {
                    throw std::runtime_error("--symbols requires a path");
                }
                opts.symbols_file = argv[i];
            } else if (arg == "--remote") {
                if (++i >= argc) {
                    throw std::runtime_error("--remote requires host:port");
                }
                opts.remote_target = argv[i];
            } else if (arg == "-ex") {
                if (++i >= argc) {
                    throw std::runtime_error("-ex requires a command");
                }
                opts.execute_commands.push_back(argv[i]);
            } else if (arg == "--nx" || arg == "-nx") {
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
    try {
        const auto opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help();
            return 0;
        }

        debugger_session session;
        if (opts.exec_file.has_value()) {
            session.set_exec_path(opts.exec_file.value());
        }
        if (opts.symbols_file.has_value()) {
            session.load_symbols_file(opts.symbols_file.value());
        } else {
            session.maybe_load_default_symbols();
        }
        if (opts.remote_target.has_value()) {
            session.connect_remote(opts.remote_target.value());
        }

        std::unique_ptr<debug_protocol> ui;
        if (opts.mode == frontend_mode::mi) {
            ui = make_mi_protocol(session);
        } else if (opts.mode == frontend_mode::dap) {
            ui = make_dap_protocol(session);
        } else {
            auto cli = std::make_unique<cli_frontend>(
                session, std::cin, std::cout, std::cerr, opts.quiet, true);
            cli->set_execute_commands(opts.execute_commands);
            ui = std::move(cli);
        }

        return ui->run();
    } catch (const std::exception& e) {
        std::cerr << "xdbg: " << e.what() << "\n";
        return 1;
    }
}
