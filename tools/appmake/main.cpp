#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

#include "appmake/cli.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc > 1 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty()) {
        appmake::print_usage();
        return 1;
    }

    const std::string_view cmd = args[0];

    try {
        if (cmd == "list") {
            if (args.size() != 2) {
                std::cerr << "list requires <tape.tap|tape.tzx>\n";
                return 1;
            }
            appmake::cmd_list(fs::path(std::string(args[1])));
            return 0;
        }

        if (cmd == "tap") {
            if (args.size() < 3) {
                std::cerr << "tap requires <input.tap> <output.app>\n";
                return 1;
            }
            appmake::cmd_tap(fs::path(std::string(args[1])),
                             fs::path(std::string(args[2])),
                             appmake::parse_options(args, 3));
            return 0;
        }

        if (cmd == "make") {
            if (args.size() < 3) {
                std::cerr << "make requires <input.tap|input.tzx> <cart.mdr>\n";
                return 1;
            }
            appmake::cmd_make(fs::path(std::string(args[1])),
                              fs::path(std::string(args[2])),
                              appmake::parse_options(args, 3));
            return 0;
        }

        if (cmd == "sna") {
            if (args.size() < 3) {
                std::cerr << "sna requires <input.sna> <output.app>\n";
                return 1;
            }
            appmake::cmd_sna(fs::path(std::string(args[1])),
                             fs::path(std::string(args[2])),
                             appmake::parse_options(args, 3));
            return 0;
        }

        if (cmd == "analyze") {
            if (args.size() != 2) {
                std::cerr << "analyze requires <input.tap>\n";
                return 1;
            }
            appmake::cmd_analyze(fs::path(std::string(args[1])));
            return 0;
        }

        std::cerr << std::format("unknown command: {}\n", cmd);
        appmake::print_usage();
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << std::format("error: {}\n", ex.what());
        return 2;
    }
}
