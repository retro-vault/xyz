//
// options.h — command-line option parsing for the xcc driver.
//
// Defines the options struct that holds the parsed state of all
// command-line flags, and the opt_level / output_mode enumerations.
//
// options::parse() is the single entry point: it reads argc/argv,
// populates an options instance, and exits with a usage message if
// mandatory arguments are missing.  The driver (main.cpp) calls this
// once and passes the result to compile_file().
//
// xcc aims for GCC-compatible flags where practical so it can be
// used as a drop-in assembler directive in existing makefiles.
// Unknown flags produce a warning but do not abort parsing.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "opt/opt_settings.h"
#include <string>
#include <vector>

namespace xcc {

//
// Target assembler dialect.  Selected with --mode=<name>.
//
enum class asm_dialect {
    SDASZ80, // SDCC sdasz80 (default)
    GNUAS,   // GNU binutils as
};

//
// Output mode selected by the driver flag.
//
enum class output_mode {
    ASSEMBLY,    // -S: stop after compilation, emit .s
    OBJECT,      // -c: compile and assemble (via xas), emit .rel
    LINK,        // default: compile, assemble, and link (via xld)
};

struct options {
    // Input / output
    std::vector<std::string> input_files;
    std::string              output_file;
    output_mode              mode      = output_mode::LINK;

    // Optimization
    opt_level                opt       = opt_level::O0;
    optimization_settings    opt_settings = optimization_settings::for_level(opt_level::O0);

    // Assembler dialect
    asm_dialect              dialect   = asm_dialect::SDASZ80;

    // Language standard (only C11 is supported)
    bool                     std_c11   = true;
    std::string              invocation_target;
    std::string              platform_name;

    // Include paths and defines forwarded to the external preprocessor
    std::vector<std::string> include_paths;
    std::vector<std::string> defines;

    bool                     debug = false; // -g: emit debug info (DWARF 2 for gnuas, SDCC ;! for sdasz80)
    bool                     dump_ir = false; // --dump-ir: print lowered IR to stderr

    // Use IX as the frame pointer (always true for now)
    bool                     use_ix_fp = true;

    bool                     verbose   = false;
    int                      max_errors = 20;

    // Flags forwarded verbatim to xld in link mode (-L, -l, -nostdlib,
    // -Wl,... and friends), in command-line order.
    std::vector<std::string> linker_args;

    // Directory containing the xcc executable; xas and xld are looked
    // up here first, then in PATH.
    std::string              driver_dir;

    //
    // Parse command-line arguments and return a populated options.
    // Calls exit(1) if no input files are given or a required argument
    // is missing.
    //
    static options parse(int argc, char **argv);

    //
    // Print usage information to stderr.
    // argv0 is the program name (argv[0]).
    //
    void usage(const char *argv0);
};

} // namespace xcc
