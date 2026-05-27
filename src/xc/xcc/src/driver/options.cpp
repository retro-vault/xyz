//
// options.cpp — command-line option parsing for the xcc driver.
//
// Implements options::parse() which processes argc/argv left to right,
// recognising GCC-compatible flags (-O, -I, -D, -o, -S, -v, etc.).
// Unknown flags produce a warning but do not abort, for compatibility
// with build systems that pass extra flags unconditionally.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "driver/options.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace xcc {

void options::usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [options] <input.c> [-o <output>]\n"
        "\n"
        "X C Compiler (xcc) — C11 compiler for Z80\n"
        "\n"
        "options:\n"
        "  -o <file>         Output file (default: <input>.s)\n"
        "  -S                Emit assembly (default)\n"
        "  -O0               No optimization (default)\n"
        "  -O1               Enable peephole optimizer\n"
        "  -O2               Enable advanced optimizations\n"
        "  -I<dir>           Add include directory\n"
        "  -D<macro>[=val]   Define preprocessor macro\n"
        "  -std=c11          Language standard (only c11 supported)\n"
        "  -g                Emit debug info\n"
        "  -masm=sdasz80     Output for SDCC sdasz80 assembler (default)\n"
        "  -masm=gnuas       Output for GNU binutils assembler\n"
        "  -v                Verbose output\n"
        "  --version         Print version\n"
        "  --help            Print this help\n",
        argv0);
}

options options::parse(int argc, char **argv) {
    options opts;

    if (argc < 2) {
        opts.usage(argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];

        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            opts.usage(argv[0]);
            exit(0);
        }
        if (strcmp(a, "--version") == 0) {
            fprintf(stdout, "xcc 0.1.0 (X C Compiler for Z80)\n");
            exit(0);
        }
        if (strcmp(a, "-S") == 0) {
            opts.mode = output_mode::ASSEMBLY;
        } else if (strncmp(a, "-o", 2) == 0) {
            if (a[2] != '\0') {
                opts.output_file = a + 2;
            } else if (i + 1 < argc) {
                opts.output_file = argv[++i];
            } else {
                fprintf(stderr, "xcc: error: -o requires a filename\n");
                exit(1);
            }
        } else if (strcmp(a, "-O0") == 0) {
            opts.opt = opt_level::O0;
        } else if (strcmp(a, "-O1") == 0) {
            opts.opt = opt_level::O1;
        } else if (strcmp(a, "-O2") == 0) {
            opts.opt = opt_level::O2;
        } else if (strncmp(a, "-I", 2) == 0) {
            opts.include_paths.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strncmp(a, "-D", 2) == 0) {
            opts.defines.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strncmp(a, "-std=", 5) == 0) {
            // We only support c11; silently accept c99/c11/gnu11 etc.
        } else if (strncmp(a, "-masm=", 6) == 0) {
            const char *dialect = a + 6;
            if (strcmp(dialect, "gnuas") == 0)
                opts.dialect = asm_dialect::GNUAS;
            else if (strcmp(dialect, "sdasz80") == 0)
                opts.dialect = asm_dialect::SDASZ80;
            else
                fprintf(stderr, "xcc: warning: unknown assembler dialect '%s'\n", dialect);
        } else if (strcmp(a, "-g") == 0) {
            opts.debug = true;
        } else if (strcmp(a, "-v") == 0) {
            opts.verbose = true;
        } else if (a[0] == '-') {
            // Unknown flag — warn but continue (gcc-compatible)
            fprintf(stderr, "xcc: warning: unrecognized option '%s'\n", a);
        } else {
            opts.input_files.push_back(a);
        }
    }

    if (opts.input_files.empty()) {
        fprintf(stderr, "xcc: error: no input files\n");
        exit(1);
    }

    return opts;
}

} // namespace xcc
