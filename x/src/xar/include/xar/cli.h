// cli.h
//
// Command-line interface for xar, the xyz archive tool.
//
// Usage:
//   xar <operation>[modifiers] archive [members...]
//
// Operations (first positional arg, or combined flag):
//   r   Add or replace members
//   c   Create archive (suppress "creating" warning)
//   t   List archive contents
//   x   Extract members
//   d   Delete members
//
// Modifiers:
//   s   Write symbol index (currently a no-op; reserved)
//   v   Verbose output
//
// Global options:
//   --mode=sdcc   Text-index .lib format (default)
//   --mode=gnu    GNU ar binary format
//   @file         Read member paths from a response file
//   --help        Print help and exit
//   --version     Print version and exit
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAR_CLI_HPP
#define XAR_CLI_HPP

#include <string>
#include <vector>

namespace xar {

    enum class archive_mode {
        sdcc,   // text-index .lib
        gnu     // ar binary .a
    };

    enum class operation {
        add,        // r — add/replace
        create,     // rc — create (alias for add with no-warn)
        list,       // t — list
        extract,    // x — extract
        remove      // d — delete
    };

    struct cli_options {
        operation       op         = operation::add;
        archive_mode    mode       = archive_mode::sdcc;
        bool            verbose    = false;
        bool            create     = false;
        std::string     archive;
        std::vector<std::string> members;
    };

    //
    // Parse command-line arguments.
    //
    cli_options parse_args(int argc, char** argv);

    [[noreturn]] void print_usage_and_exit(const char* prog, int code = 1);

} // namespace xar

#endif // XAR_CLI_HPP
