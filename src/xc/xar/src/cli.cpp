// cli.cpp
//
// xar command-line argument parser.
// Supports both traditional positional flags ("rcs archive.lib file.rel")
// and separate flag groups ("--mode=gnu r archive.a file.o").
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <xar/cli.hpp>

namespace xar {

    [[noreturn]] void print_usage_and_exit(const char* prog, int code)
    {
        std::cerr
            << "Usage: " << prog << " [--mode=sdcc|gnu] <operation> archive [members...]\n"
            << "\n"
            << "X Archiver (xar) — Z80 archive tool\n"
            << "\n"
            << "operations:\n"
            << "  r   Add or replace members\n"
            << "  t   List archive contents\n"
            << "  x   Extract members (all if none specified)\n"
            << "  d   Delete members\n"
            << "\n"
            << "modifiers (combine with operation letter):\n"
            << "  c   Create archive (suppress warning)\n"
            << "  v   Verbose\n"
            << "  s   Write symbol index (reserved, no-op)\n"
            << "\n"
            << "options:\n"
            << "  --mode=sdcc   Text-index .lib format (default)\n"
            << "  --mode=gnu    GNU ar binary format\n"
            << "  --help        Show this help\n"
            << "  --version     Show version\n"
            << "\n"
            << "examples:\n"
            << "  xar rcs mylib.lib foo.rel bar.rel\n"
            << "  xar --mode=gnu rc mylib.a foo.o bar.o\n"
            << "  xar t mylib.lib\n"
            << "  xar x mylib.lib foo.rel\n"
            << "  xar d mylib.lib old.rel\n";
        std::exit(code);
    }

    cli_options parse_args(int argc, char** argv)
    {
        cli_options opts;
        const char* prog = argc > 0 ? argv[0] : "xar";
        int i = 1;

        if (argc < 2)
            print_usage_and_exit(prog, 1);

        // Process --options first.
        for (; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") print_usage_and_exit(prog, 0);
            if (arg == "--version") {
                std::cout << "xar 0.1.0\n";
                std::exit(0);
            }
            if (arg == "--mode=sdcc") { opts.mode = archive_mode::sdcc; continue; }
            if (arg == "--mode=gnu")  { opts.mode = archive_mode::gnu;  continue; }
            if (arg[0] != '-') break; // positional argument
            throw std::runtime_error("unknown option: " + arg);
        }

        if (i >= argc)
            print_usage_and_exit(prog, 1);

        // Parse operation string (e.g. "rcs", "t", "xv").
        std::string opstr = argv[i++];
        operation op_found = operation::add;
        bool op_set = false;

        for (char c : opstr) {
            switch (c) {
                case 'r': op_found = operation::add;     op_set = true; break;
                case 't': op_found = operation::list;    op_set = true; break;
                case 'x': op_found = operation::extract; op_set = true; break;
                case 'd': op_found = operation::remove;  op_set = true; break;
                case 'c': opts.create  = true; break;
                case 'v': opts.verbose = true; break;
                case 's': break; // symbol index — reserved
                default:
                    throw std::runtime_error(
                        std::string("unknown operation modifier: ") + c);
            }
        }
        if (!op_set)
            throw std::runtime_error("no operation in: " + opstr);

        opts.op = op_found;

        if (i >= argc)
            throw std::runtime_error("no archive name specified");

        opts.archive = argv[i++];

        while (i < argc)
            opts.members.push_back(argv[i++]);

        return opts;
    }

} // namespace xar
