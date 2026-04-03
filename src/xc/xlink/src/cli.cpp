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

    static uint16_t parse_hex16(const std::string& s) {
        return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
    }

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
            } else if (arg == "-n") {
                if (++i >= argc)
                    throw xlink_error("-n requires an argument");
                opts.symbol_file = std::filesystem::path(argv[i]);
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
                r.start = parse_hex16(range_str.substr(0, dash));
                r.end = parse_hex16(range_str.substr(dash + 1));
                opts.reserved_ranges.push_back(r);
            } else if (arg == "-b") {
                if (++i >= argc)
                    throw xlink_error("-b requires an argument");
                std::string base_str = argv[i];
                auto eq = base_str.find('=');
                if (eq == std::string::npos || eq == 0
                    || eq == base_str.size() - 1)
                    throw xlink_error(
                        "-b format: AREA=ADDR (hex), e.g. _CODE=0100");
                std::string area_name = base_str.substr(0, eq);
                uint16_t base = parse_hex16(base_str.substr(eq + 1));
                opts.area_bases[area_name] = base;
            } else if (arg == "-f") {
                if (++i >= argc)
                    throw xlink_error("-f requires an argument");
                std::string format = argv[i];
                if (format == "xl") {
                    opts.format = output_format::xl;
                } else if (format == "bin") {
                    opts.format = output_format::bin;
                } else {
                    throw xlink_error("unsupported output format: " + format);
                }
            } else if (arg == "-x") {
                if (++i >= argc)
                    throw xlink_error("-x requires an argument");
                std::string range_str = argv[i];
                auto dash = range_str.find('-');
                if (dash == std::string::npos)
                    throw xlink_error(
                        "-x format: start-end (hex), e.g. 0000-3FFF");
                address_range r;
                r.start = parse_hex16(range_str.substr(0, dash));
                r.end = parse_hex16(range_str.substr(dash + 1));
                opts.output_range = r;
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
            << "  -n <file>         write DEF symbol file\n"
            << "  -e <symbol>       entry point symbol (default: _main)\n"
            << "  -r <start>-<end>  reserve address range (hex), repeatable\n"
            << "  -b <area>=<addr>  set base address for area group (hex)\n"
            << "  -f <xl|bin>       output format (default: xl)\n"
            << "  -x <start>-<end>  output range for -f bin (hex, inclusive)\n"
            << "  -m                print memory map after linking\n"
            << "  -v                verbose output\n"
            << "  -h, --help        show this help\n";
    }

} // namespace xlink
