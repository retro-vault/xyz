// cli.cpp
//
// xas command-line argument parser.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include <xas/cli.h>

namespace xas {

#ifndef XTOOLS_VERSION
#define XTOOLS_VERSION "0.1.0"
#endif

    [[noreturn]] void print_usage_and_exit(const char* prog, int code)
    {
        std::cerr
            << "Usage: " << prog << " [options] <input.s>\n"
            << "\n"
            << "X Tools Assembler (xas) — Z80 assembler\n"
            << "\n"
            << "options:\n"
            << "  --mode=sdcc               SDCC sdasz80 directives, .rel output (default)\n"
            << "  --mode=gnu                GNU gas directives, ELF32 output\n"
            << "  --format=sdcc             Pretty-print / emit SDCC-style assembly text\n"
            << "  --format=gnu              Pretty-print / emit GNU-style assembly text\n"
            << "                            Supported source subset excludes assembler macros\n"
            << "  -o <file>                 Output file\n"
            << "  -g                        Emit debug information\n"
            << "  -I <dir>, -I<dir>         Add include directory\n"
            << "  -D <sym[=v]>, -D<sym[=v]> Define preprocessor symbol\n"
            << "  --version                 Print version\n"
            << "  -h, --help                Show this help\n";
        std::exit(code);
    }

    cli_options parse_args(int argc, char** argv)
    {
        cli_options opts;
        const char* prog = argc > 0 ? argv[0] : "xas";

        if (argc < 2)
            print_usage_and_exit(prog, 1);

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                print_usage_and_exit(prog, 0);
            }
            if (arg == "--version") {
                std::cout << "xas " << XTOOLS_VERSION
                          << " (X Tools Assembler for Z80)\n";
                std::exit(0);
            }
            if (arg == "--mode=sdcc") {
                opts.mode = asm_mode::sdcc;
            } else if (arg == "--mode=gnu") {
                opts.mode = asm_mode::gnu;
            } else if (arg == "--format=sdcc") {
                opts.format = output_format::sdcc;
            } else if (arg == "--format=gnu") {
                opts.format = output_format::gnu;
            } else if (arg == "-g") {
                opts.debug = true;
            } else if (arg == "-o") {
                if (++i >= argc)
                    throw std::runtime_error("-o requires an argument");
                opts.output = argv[i];
            } else if (arg.rfind("-o", 0) == 0) {
                opts.output = arg.substr(2);
            } else if (arg == "-I") {
                if (++i >= argc)
                    throw std::runtime_error("-I requires an argument");
                opts.include_dirs.push_back(argv[i]);
            } else if (arg.rfind("-I", 0) == 0) {
                opts.include_dirs.push_back(arg.substr(2));
            } else if (arg == "-D") {
                if (++i >= argc)
                    throw std::runtime_error("-D requires an argument");
                opts.defines.push_back(argv[i]);
            } else if (arg.rfind("-D", 0) == 0) {
                opts.defines.push_back(arg.substr(2));
            } else if (arg[0] == '-') {
                throw std::runtime_error("unknown option: " + arg);
            } else {
                if (!opts.input.empty())
                    throw std::runtime_error("only one input file supported");
                opts.input = arg;
            }
        }

        if (opts.input.empty())
            print_usage_and_exit(prog, 1);

        if (opts.format && opts.output.empty())
            throw std::runtime_error("--format requires -o <file>");

        if (opts.output.empty()) {
            std::filesystem::path p(opts.input);
            opts.output = p.stem().string()
                          + (opts.mode == asm_mode::gnu ? ".o" : ".rel");
        }

        return opts;
    }

} // namespace xas
