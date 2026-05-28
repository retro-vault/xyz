// cli.hpp
//
// Command-line interface for xas.  Parses argc/argv and exposes the
// resulting options.  Mirrors xcc and xlink option naming where possible.
//
// Usage:
//   xas [options] <input.s>
//
//   --mode=sdcc   SDCC sdasz80 directives; emit .rel object (default)
//   --mode=gnu    GNU gas directives; emit ELF32 z80-elf object
//   -o <file>     Output file (default: input stem + .rel or .o)
//   -g            Emit debug information
//   -I <dir>      Add include search directory
//   -D <sym[=v]>  Define preprocessor symbol
//   --help        Print help and exit
//   --version     Print version and exit
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_CLI_HPP
#define XAS_CLI_HPP

#include <string>
#include <vector>

namespace xas {

    enum class asm_mode {
        sdcc,   // SDCC sdasz80 directives + .rel output
        gnu     // GNU gas directives + ELF output
    };

    struct cli_options {
        asm_mode            mode      = asm_mode::sdcc;
        std::string         input;
        std::string         output;
        bool                debug     = false;
        std::vector<std::string> include_dirs;
        std::vector<std::string> defines;
    };

    //
    // Parse command-line arguments.  Returns options on success.
    // Prints usage and exits on --help / --version / bad arguments.
    //
    cli_options parse_args(int argc, char** argv);

    //
    [[noreturn]] void print_usage_and_exit(const char* prog, int code = 1);

} // namespace xas

#endif // XAS_CLI_HPP
