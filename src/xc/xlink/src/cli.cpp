// cli.cpp
//
// command-line argument parsing
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <xlink/cli.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    cli_options cli::parse(int argc, char* argv[]) {
        cli_options opts;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                opts.show_help = true;
                return opts;
            } else if (arg == "-o") {
                if (++i >= argc)
                    throw xlink_error("-o requires an argument");
                opts.output_file = argv[i];
            } else if (arg == "-e") {
                if (++i >= argc)
                    throw xlink_error("-e requires an argument");
                opts.entry_symbol = argv[i];
            } else if (arg == "-r") {
                if (++i >= argc)
                    throw xlink_error("-r requires an argument");
                // Parse "start-end" in hex.
                std::string range_str = argv[i];
                auto dash = range_str.find('-');
                if (dash == std::string::npos)
                    throw xlink_error(
                        "-r format: start-end (hex), e.g. 4000-7FFF");
                address_range r;
                r.start = static_cast<uint16_t>(
                    std::stoul(range_str.substr(0, dash), nullptr, 16));
                r.end = static_cast<uint16_t>(
                    std::stoul(range_str.substr(dash + 1), nullptr, 16));
                opts.reserved_ranges.push_back(r);
            } else if (arg == "-m") {
                opts.print_map = true;
            } else if (arg == "-v") {
                opts.verbose = true;
            } else if (arg[0] == '-') {
                throw xlink_error("unknown option: " + arg);
            } else {
                opts.input_files.emplace_back(arg);
            }
        }

        if (!opts.show_help && opts.input_files.empty())
            throw xlink_error("no input files");

        return opts;
    }

    void cli::print_usage() {
        std::cout
            << "xlink - Z80 linker for xyz\n"
            << "usage: xlink [options] <file.rel|file.lib> ...\n\n"
            << "options:\n"
            << "  -o <file>         output file (default: a.out)\n"
            << "  -e <symbol>       entry point symbol (default: _main)\n"
            << "  -r <start>-<end>  reserve address range (hex), repeatable\n"
            << "  -m                print memory map after linking\n"
            << "  -v                verbose output\n"
            << "  -h, --help        show this help\n";
    }

} // namespace xlink
