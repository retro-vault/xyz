//
// This file implements the command-line options parser for the assembler,
// handling flags like -o, -c, -g, -I, -D, -v, and -masm.
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#include <getopt.h>
#include <iostream>

#include "options.h"

namespace xas
{

    options parse_args(int argc, char **argv)
    {
        options opts;
        optind = 1; // ← reset to start of argv[]
        opterr = 0; // ← suppress default error messages if you like

        // Define long options for getopt_long.
        const struct option longopts[] = {
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'V'},
            {"output", required_argument, 0, 'o'},
            {"compile-only", no_argument, 0, 'c'},
            {"debug", no_argument, 0, 'g'},
            {"include", required_argument, 0, 'I'},
            {"define", required_argument, 0, 'D'},
            {"verbose", no_argument, 0, 'v'},
            {"masm", required_argument, 0, 'm'},
            {0, 0, 0, 0}};

        // Short options string: hVo:cgI:D:vm:
        const char *shortopts = "hVo:cgI:D:vm:";

        int c;
        // Parse each option
        while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) != -1)
        {
            switch (c)
            {
            case 'h':
                // Print usage and exit
                std::cout << "Usage: xas [options] file.s\n"
                          << "  -m, --masm <name>   select assembler dialect (sdas, zilog).\n";
                std::exit(0);
            case 'V':
                // Print version and exit
                std::cout << "xas version 1.0\n";
                std::exit(0);
            case 'o':
                opts.output_file = optarg;
                break;
            case 'c':
                opts.compile_only = true;
                break;
            case 'g':
                opts.debug = true;
                break;
            case 'I':
                // Add include directory
                opts.include_dirs.emplace_back(optarg);
                break;
            case 'D':
                // Add predefined macro
                opts.defines.emplace_back(optarg);
                break;
            case 'v':
                opts.verbose = true;
                break;
            case 'm':
                // Set assembler dialect
                opts.masm = optarg;
                break;
            default:
                std::cerr << "Unknown option\n";
                std::exit(1);
            }
        }

        // Remaining arguments are input files
        for (int i = optind; i < argc; ++i)
        {
            opts.input_files.emplace_back(argv[i]);
        }
        if (opts.input_files.empty())
        {
            std::cerr << "Error: no input files specified\n";
            std::exit(1);
        }

        return opts;
    }

} // namespace xas