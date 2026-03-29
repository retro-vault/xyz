//
// This file parses command-line options for the assembler,
// handling include paths, defines, dialect selection, etc.
//
// NOTES:
//   Gnu assembler compatible args.
//
// USAGE:
//   options opts = parse_args(argc, argv);
//   for (auto& in : opts.input_files) { /* assemble each file */ }
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#pragma once
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

namespace xas
{

    /// Parsed command-line options for the assembler.
    struct options
    {
        std::vector<std::string> input_files;  ///< List of input source filenames.
        std::string output_file;               ///< Output object filename.
        bool compile_only = false;             ///< Only assemble; do not link.
        bool debug = false;                    ///< Generate debug symbols.
        std::vector<std::string> include_dirs; ///< Directories to search for includes.
        std::vector<std::string> defines;      ///< Predefined macros (e.g., -D).
        bool verbose = false;                  ///< Enable verbose output.
        std::string masm;                      ///< Assembler dialect (e.g., pasmo, z80asm).
    };

    /// Parse command-line arguments into an options struct.
    /// @param argc Argument count.
    /// @param argv Argument vector.
    /// @return Populated options.
    extern options parse_args(int argc, char **argv);

} // namespace xas