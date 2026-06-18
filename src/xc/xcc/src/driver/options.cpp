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
#include <filesystem>
#include <system_error>

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

static std::filesystem::path normalize_path(const std::filesystem::path &path) {
    std::error_code ec;
    auto normalized = std::filesystem::weakly_canonical(path, ec);
    if (!ec)
        return normalized;
    return path.lexically_normal();
}

static std::filesystem::path resolve_process_executable(const char *argv0) {
    std::error_code ec;
    auto proc_self = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !proc_self.empty())
        return normalize_path(proc_self);

    std::filesystem::path executable = (argv0 && *argv0) ? argv0 : "xcc";
    if (executable.is_relative()) {
        executable = std::filesystem::absolute(executable, ec);
        if (ec)
            executable = std::filesystem::path((argv0 && *argv0) ? argv0 : "xcc");
    }
    return normalize_path(executable);
}

static std::string normalize_target_name(std::string target_name) {
    if (target_name.empty())
        return target_name;
    if (target_name.rfind("z80-", 0) == 0)
        return target_name;
    return "z80-" + target_name;
}

static std::string detect_invocation_target(const char *argv0,
                                            const char *tool_name) {
    if (argv0 == nullptr || *argv0 == '\0')
        return {};

    const std::string base = std::filesystem::path(argv0).filename().string();
    const std::string suffix = std::string("-") + tool_name;
    if (base.size() <= suffix.size())
        return {};
    if (base.compare(base.size() - suffix.size(), suffix.size(), suffix) != 0)
        return {};
    return base.substr(0, base.size() - suffix.size());
}

static void add_default_include_path(options &opts,
                                     const std::filesystem::path &path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec))
        return;

    const auto normalized = normalize_path(path).string();
    for (const auto &existing : opts.include_paths) {
        if (existing == normalized)
            return;
    }
    opts.include_paths.push_back(normalized);
}

static void add_default_include_paths(options &opts, const char *argv0) {
    const auto executable = resolve_process_executable(argv0);
    const auto prefix = executable.parent_path().parent_path();

    // GNU cross-toolchain prefix layout: target headers live under
    // <prefix>/z80/include; <prefix>/include holds host SDK headers only.
    add_default_include_path(opts, prefix / "z80" / "include");
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
        "Usage: %s [options] <input>... [-o <output>]\n"
        "\n"
        "X C Compiler (xcc) — C11 compiler driver for Z80\n"
        "\n"
        "options:\n"
        "  -o <file>         Output file\n"
        "  -c                Compile and assemble only, emit .rel\n"
        "  -S                Compile only, emit assembly\n"
        "  -O0               No optimization (default)\n"
        "  -O1               Enable peephole optimizer\n"
        "  -O2               Enable general optimization\n"
        "  -Of               Enable speed optimization\n"
        "  -O3               Enable experimental optimization (Here be dragons)\n"
        "  -Os               Enable size optimization\n"
        "  -f<name>          Enable one optimization family\n"
        "  -fno-<name>       Disable one optimization family\n"
        "  -I<dir>           Add include directory\n"
        "  -D<macro>[=val]   Define preprocessor macro\n"
        "  -std=c11          Language standard (only c11 supported)\n"
        "  -masm=<dialect>   Assembler dialect: sdasz80 (default) or gnuas\n"
        "  --platform=<name> Select target platform include defaults\n"
        "  -g                Emit debug info\n"
        "  --dump-ir         Dump lowered IR to stderr\n"
        "  --mode=sdcc       Output for SDCC sdasz80 assembler (default)\n"
        "  --mode=gnu        Output for GNU binutils assembler\n"
        "  -L<dir>, -l<name> Forwarded to the linker\n"
        "  -nostdlib         Forwarded to the linker\n"
        "  -nostartfiles     Forwarded to the linker\n"
        "  --oformat=<fmt>   Forwarded to the linker (xl, binary, elf, ihx)\n"
        "  -T*, -e <sym>     Forwarded to the linker\n"
        "  -Wl,<args>        Forward comma-separated args to the linker\n"
        "  -v                Verbose output\n"
        "  --version         Print version\n"
        "  --help            Print this help\n",
        argv0);
}

options options::parse(int argc, char **argv) {
    options opts;
    opts.invocation_target = detect_invocation_target(
        argc > 0 ? argv[0] : nullptr, "xcc");

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
        } else if (strcmp(a, "-c") == 0) {
            opts.mode = output_mode::OBJECT;
        } else if (strncmp(a, "-Wl,", 4) == 0) {
            // Comma-separated arguments forwarded verbatim to the linker.
            const char *p = a + 4;
            while (*p) {
                const char *comma = strchr(p, ',');
                if (!comma) {
                    opts.linker_args.emplace_back(p);
                    break;
                }
                opts.linker_args.emplace_back(p, comma - p);
                p = comma + 1;
            }
        } else if (strncmp(a, "-L", 2) == 0 || strncmp(a, "-l", 2) == 0
                   || strcmp(a, "-nostdlib") == 0
                   || strcmp(a, "-nostartfiles") == 0
                   || strcmp(a, "--no-default-runtime") == 0
                   || strncmp(a, "--oformat=", 10) == 0
                   || strncmp(a, "-Ttext=", 7) == 0
                   || strncmp(a, "-Tdata=", 7) == 0
                   || strncmp(a, "-Tbss=", 6) == 0
                   || strncmp(a, "--section-start=", 16) == 0
                   || strncmp(a, "--script=", 9) == 0
                   || strncmp(a, "-Map=", 5) == 0
                   || strncmp(a, "--binary-range=", 15) == 0
                   || strncmp(a, "--reserve=", 10) == 0) {
            opts.linker_args.emplace_back(a);
        } else if (strcmp(a, "-T") == 0 || strcmp(a, "-e") == 0
                   || strcmp(a, "-B") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: %s requires an argument\n", a);
                exit(1);
            }
            opts.linker_args.emplace_back(a);
            opts.linker_args.emplace_back(argv[++i]);
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
        } else if (strcmp(a, "--platform") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: --platform requires a value\n");
                exit(1);
            }
            opts.platform_name = normalize_target_name(argv[++i]);
        } else if (strncmp(a, "--platform=", 11) == 0) {
            opts.platform_name = normalize_target_name(a + 11);
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

    add_default_include_paths(opts, argv[0]);
    opts.driver_dir = resolve_process_executable(argv[0]).parent_path().string();

    return opts;
}

} // namespace xcc
