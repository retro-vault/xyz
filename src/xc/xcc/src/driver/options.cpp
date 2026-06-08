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

namespace {

struct opt_flag_binding {
    const char *name;
    bool optimization_settings::*member;
};

static constexpr opt_flag_binding k_opt_flag_bindings[] = {
    {"peephole", &optimization_settings::peephole},
    {"dead-static-functions", &optimization_settings::dead_static_functions},
    {"const-arg-prop", &optimization_settings::const_arg_propagation},
    {"const-call-eval", &optimization_settings::const_call_eval},
    {"function-const-eval", &optimization_settings::function_const_eval},
    {"dead-params", &optimization_settings::dead_params},
    {"merge-identical-functions", &optimization_settings::merge_identical_functions},
    {"inline-static-functions", &optimization_settings::inline_static_functions},
    {"cfg-cleanup", &optimization_settings::cfg_cleanup},
    {"jump-threading", &optimization_settings::jump_threading},
    {"address-deref-fold", &optimization_settings::address_deref_fold},
    {"value-propagation", &optimization_settings::value_propagation},
    {"constant-fold", &optimization_settings::constant_folding},
    {"algebraic-simplify", &optimization_settings::algebraic_simplify},
    {"local-cse", &optimization_settings::local_cse},
    {"loop-licm", &optimization_settings::loop_licm},
    {"loop-induction", &optimization_settings::loop_induction},
    {"strength-reduction", &optimization_settings::strength_reduction},
    {"dead-code-elim", &optimization_settings::dead_code_elim},
    {"scalar-local-promotion", &optimization_settings::scalar_local_promotion},
    {"reg-param-promotion", &optimization_settings::reg_param_promotion},
    {"short-circuit-bool-ifx", &optimization_settings::short_circuit_bool_ifx},
    {"narrow-counted-byte-loops", &optimization_settings::narrow_counted_byte_loops},
    {"loop-pointer-walk", &optimization_settings::loop_pointer_walk},
    {"promoted-byte-compare", &optimization_settings::promoted_byte_compare},
    {"promoted-byte-ops", &optimization_settings::promoted_byte_ops},
    {"rotate-combine", &optimization_settings::rotate_combine},
    {"duplicate-block-merge", &optimization_settings::duplicate_block_merge},
    {"merge-tails", &optimization_settings::merge_tails},
    {"local-frame-compaction", &optimization_settings::local_frame_compaction},
    {"regalloc", &optimization_settings::regalloc},
    {"compare-ifx-fusion", &optimization_settings::compare_ifx_fusion},
    {"frame-omit", &optimization_settings::frame_omit},
    {"prealloc-temp-frame", &optimization_settings::prealloc_temp_frame},
    {"switch-jump-tables", &optimization_settings::switch_jump_tables},
};

static bool apply_opt_flag(options &opts, const char *name, bool enabled) {
    for (const auto &binding : k_opt_flag_bindings) {
        if (strcmp(binding.name, name) == 0) {
            opts.opt_settings.*(binding.member) = enabled;
            return true;
        }
    }
    return false;
}

} // namespace

static bool apply_asm_dialect_option(options& opts, const char *mode) {
    if (strcmp(mode, "gnu") == 0 || strcmp(mode, "gnuas") == 0) {
        opts.dialect = asm_dialect::GNUAS;
        return true;
    }
    if (strcmp(mode, "sdcc") == 0 || strcmp(mode, "sdasz80") == 0) {
        opts.dialect = asm_dialect::SDASZ80;
        return true;
    }
    return false;
}

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
        "  -O2               Enable general optimization\n"
        "  -Of               Enable speed optimization\n"
        "  -O3               Enable experimental optimization\n"
        "  -Os               Enable size optimization\n"
        "  -f<name>          Enable one optimization family\n"
        "  -fno-<name>       Disable one optimization family\n"
        "  -I<dir>           Add include directory\n"
        "  -D<macro>[=val]   Define preprocessor macro\n"
        "  -std=c11          Language standard (only c11 supported)\n"
        "  -masm=<dialect>   Assembler dialect: sdasz80 (default) or gnuas\n"
        "  -g                Emit debug info\n"
        "  --dump-ir         Dump lowered IR to stderr\n"
        "  --mode=sdcc       Output for SDCC sdasz80 assembler (default)\n"
        "  --mode=gnu        Output for GNU binutils assembler\n"
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
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "-O1") == 0) {
            opts.opt = opt_level::O1;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "-O2") == 0) {
            opts.opt = opt_level::O2;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "-Of") == 0) {
            opts.opt = opt_level::Of;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "-O3") == 0) {
            opts.opt = opt_level::O3;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "-Os") == 0) {
            opts.opt = opt_level::Os;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strncmp(a, "-fno-", 5) == 0) {
            const char *name = a + 5;
            if (!apply_opt_flag(opts, name, false))
                fprintf(stderr, "xcc: warning: unknown optimization flag '%s'\n", a);
        } else if (strncmp(a, "-f", 2) == 0 && a[2] != '\0') {
            const char *name = a + 2;
            if (!apply_opt_flag(opts, name, true))
                fprintf(stderr, "xcc: warning: unknown optimization flag '%s'\n", a);
        } else if (strncmp(a, "-I", 2) == 0) {
            opts.include_paths.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strncmp(a, "-D", 2) == 0) {
            opts.defines.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strncmp(a, "-std=", 5) == 0) {
            // We only support c11; silently accept c99/c11/gnu11 etc.
        } else if (strncmp(a, "-masm=", 6) == 0) {
            const char *mode = a + 6;
            if (!apply_asm_dialect_option(opts, mode))
                fprintf(stderr, "xcc: warning: unknown assembler dialect '%s'\n", mode);
        } else if (strcmp(a, "-masm") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: -masm requires a dialect\n");
                exit(1);
            }
            const char *mode = argv[++i];
            if (!apply_asm_dialect_option(opts, mode))
                fprintf(stderr, "xcc: warning: unknown assembler dialect '%s'\n", mode);
        } else if (strncmp(a, "--mode=", 7) == 0) {
            const char *mode = a + 7;
            if (!apply_asm_dialect_option(opts, mode))
                fprintf(stderr, "xcc: warning: unknown mode '%s'\n", mode);
        } else if (strcmp(a, "-g") == 0) {
            opts.debug = true;
        } else if (strcmp(a, "--dump-ir") == 0) {
            opts.dump_ir = true;
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
