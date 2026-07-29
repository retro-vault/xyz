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
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef XCC_VERSION
#define XCC_VERSION "0.1.0"
#endif

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
    {"inline-trivial-internal-functions", &optimization_settings::inline_trivial_internal_functions},
    {"inline-static-functions", &optimization_settings::inline_static_functions},
    {"internal-call-abi-promotion", &optimization_settings::internal_call_abi_promotion},
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
    {"tail-recursion-elim", &optimization_settings::tail_recursion_elim},
    {"short-circuit-bool-ifx", &optimization_settings::short_circuit_bool_ifx},
    {"branch-bool-arithmetic", &optimization_settings::branch_bool_arithmetic},
    {"countdown-dead-loops", &optimization_settings::countdown_dead_loops},
    {"block-fill-loops", &optimization_settings::block_fill_loops},
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
#ifdef _WIN32
    std::vector<char> path_buf(MAX_PATH);
    for (;;) {
        const DWORD len = ::GetModuleFileNameA(
            nullptr, path_buf.data(), static_cast<DWORD>(path_buf.size()));
        if (len == 0)
            break;
        if (len < path_buf.size())
            return normalize_path(std::filesystem::path(std::string(path_buf.data(), len)));
        path_buf.resize(path_buf.size() * 2);
    }
#else
    auto proc_self = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !proc_self.empty())
        return normalize_path(proc_self);
#endif

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

static bool apply_float_format_option(options &opts, const char *format) {
    if (strcmp(format, "ieee32") == 0 || strcmp(format, "ieee") == 0 ||
        strcmp(format, "float") == 0 || strcmp(format, "soft") == 0) {
        opts.float_fmt = float_format::IEEE32;
        return true;
    }
    if (strcmp(format, "ieee16") == 0 || strcmp(format, "half") == 0 ||
        strcmp(format, "binary16") == 0) {
        opts.float_fmt = float_format::IEEE16;
        return true;
    }
    if (strcmp(format, "fixed8_8") == 0 || strcmp(format, "8_8") == 0 ||
        strcmp(format, "fixed8.8") == 0 || strcmp(format, "8.8") == 0) {
        opts.float_fmt = float_format::FIXED8_8;
        return true;
    }
    if (strcmp(format, "fixed16_16") == 0 || strcmp(format, "16_16") == 0 ||
        strcmp(format, "fixed16.16") == 0 || strcmp(format, "16.16") == 0) {
        opts.float_fmt = float_format::FIXED16_16;
        return true;
    }
    if (strcmp(format, "fixed24_8") == 0 || strcmp(format, "24_8") == 0 ||
        strcmp(format, "fixed24.8") == 0 || strcmp(format, "24.8") == 0) {
        opts.float_fmt = float_format::FIXED24_8;
        return true;
    }
    return false;
}

static bool parse_default_call_mode(const char *text, call_abi &abi) {
    if (text == nullptr)
        return false;

    if (strcmp(text, "0") == 0) {
        abi = call_abi::SDCCCALL0;
        return true;
    }
    if (strcmp(text, "1") == 0) {
        abi = call_abi::SDCCCALL1;
        return true;
    }
    return false;
}

static void add_float_format_defines(options &opts) {
    const int format_id =
        opts.float_fmt == float_format::IEEE32 ? 0 :
        opts.float_fmt == float_format::FIXED8_8 ? 1 :
        opts.float_fmt == float_format::FIXED16_16 ? 2 :
        opts.float_fmt == float_format::FIXED24_8 ? 3 : 4;
    opts.defines.push_back(std::string("__XCC_FLOAT_FORMAT_") +
                           (opts.float_fmt == float_format::IEEE32 ? "IEEE32" :
                            opts.float_fmt == float_format::IEEE16 ? "IEEE16" :
                            opts.float_fmt == float_format::FIXED8_8 ? "FIXED8_8" :
                            opts.float_fmt == float_format::FIXED16_16 ? "FIXED16_16" :
                            "FIXED24_8") + "=1");
    opts.defines.push_back(std::string("__XCC_FLOAT_FORMAT__=") +
                           std::to_string(format_id));
    opts.defines.push_back(std::string("__XCC_FLOAT_SIZE__=") +
                           std::to_string(float_format_size(opts.float_fmt)));
    opts.defines.push_back(std::string("__XCC_FLOAT_FRACTION_BITS__=") +
                           std::to_string(float_format_fraction_bits(opts.float_fmt)));
}

static void add_default_call_mode_defines(options &opts) {
    const int abi_id = opts.default_call_abi == call_abi::SDCCCALL0 ? 0 : 1;
    opts.defines.push_back(std::string("__SDCCCALL=") + std::to_string(abi_id));
}

static void add_plain_char_defines(options &opts) {
    if (!opts.plain_char_unsigned)
        return;
    opts.defines.push_back("__CHAR_UNSIGNED__=1");
    opts.defines.push_back("__SDCC_CHAR_UNSIGNED=1");
}

static std::string platform_define_name(const std::string &platform_name) {
    std::string name = "__XCC_PLATFORM_";
    for (unsigned char ch : platform_name) {
        if (std::isalnum(ch) != 0) {
            name.push_back(static_cast<char>(std::toupper(ch)));
        } else {
            name.push_back('_');
        }
    }
    return name + "=1";
}

static void driver_warning(const options &opts, warning_group group,
                           const char *fmt, ...) {
    if (!opts.diagnostics.group_enabled(group))
        return;

    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    const bool promoted = opts.diagnostics.group_as_error(group);
    fprintf(stderr, "xcc: %s: %s [-W%s%s]\n",
            promoted ? "error" : "warning",
            msg,
            promoted ? "error=" : "",
            warning_group_name(group));
    if (promoted)
        exit(1);
}

static void apply_warning_level(options &opts, int level) {
    opts.diagnostics.set_defaults();
    if (level == 0) {
        opts.diagnostics.disable_all();
    } else if (level == 2) {
        opts.diagnostics.enable_wall();
    } else if (level >= 3) {
        opts.diagnostics.enable_wall();
        opts.diagnostics.enable_wextra();
        opts.diagnostics.enable_pedantic();
    }
}

static bool apply_warning_group_option(options &opts,
                                       const std::string &name,
                                       bool enabled) {
    if (name == "all") {
        if (enabled)
            opts.diagnostics.enable_wall();
        else
            opts.diagnostics.disable_all();
        return true;
    }
    if (name == "extra") {
        if (enabled)
            opts.diagnostics.enable_wextra();
        else {
            opts.diagnostics.set_group(warning_group::OLD_STYLE_DEFINITION, false);
            opts.diagnostics.set_group(warning_group::ABI, false);
            opts.diagnostics.set_group(warning_group::CONSTEXPR_NOT_CONSTANT, false);
            opts.diagnostics.set_group(warning_group::BITINT_WIDTH, false);
        }
        return true;
    }
    if (name == "pedantic") {
        if (enabled)
            opts.diagnostics.enable_pedantic();
        else {
            opts.diagnostics.set_group(warning_group::C23_EXTENSIONS, false);
            opts.diagnostics.set_group(warning_group::BITINT_WIDTH, false);
        }
        return true;
    }

    warning_group group;
    if (!warning_group_from_name(name, group))
        return false;
    opts.diagnostics.set_group(group, enabled);
    if (!enabled)
        opts.diagnostics.set_group_error(group, false);
    return true;
}

static bool apply_warning_error_option(options &opts,
                                       const std::string &name,
                                       bool enabled) {
    if (name == "all") {
        opts.diagnostics.all_warnings_as_errors = enabled;
        return true;
    }
    warning_group group;
    if (!warning_group_from_name(name, group))
        return false;
    opts.diagnostics.set_group_error(group, enabled);
    return true;
}

static bool apply_warning_option(options &opts, const char *arg) {
    if (strcmp(arg, "-w") == 0) {
        opts.diagnostics.disable_all();
        return true;
    }
    if (strcmp(arg, "-W0") == 0 || strcmp(arg, "-W1") == 0 ||
        strcmp(arg, "-W2") == 0 || strcmp(arg, "-W3") == 0) {
        apply_warning_level(opts, arg[2] - '0');
        return true;
    }
    if (strcmp(arg, "-Wall") == 0) {
        opts.diagnostics.enable_wall();
        return true;
    }
    if (strcmp(arg, "-Wextra") == 0) {
        opts.diagnostics.enable_wextra();
        return true;
    }
    if (strcmp(arg, "-Wpedantic") == 0 || strcmp(arg, "-pedantic") == 0) {
        opts.diagnostics.enable_pedantic();
        return true;
    }
    if (strcmp(arg, "-Werror") == 0) {
        opts.diagnostics.all_warnings_as_errors = true;
        return true;
    }
    if (strcmp(arg, "-Wno-error") == 0) {
        opts.diagnostics.all_warnings_as_errors = false;
        return true;
    }
    if (strncmp(arg, "-Werror=", 8) == 0) {
        const std::string name = arg + 8;
        if (!apply_warning_error_option(opts, name, true)) {
            driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                           "unknown warning option '-Werror=%s'", name.c_str());
        }
        return true;
    }
    if (strncmp(arg, "-Wno-error=", 11) == 0) {
        const std::string name = arg + 11;
        if (!apply_warning_error_option(opts, name, false)) {
            driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                           "unknown warning option '-Wno-error=%s'", name.c_str());
        }
        return true;
    }
    if (strncmp(arg, "-Wno-", 5) == 0) {
        const std::string name = arg + 5;
        if (!apply_warning_group_option(opts, name, false)) {
            driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                           "unknown warning option '%s'", arg);
        }
        return true;
    }
    if (strncmp(arg, "-W", 2) == 0 && arg[2] != '\0') {
        const std::string name = arg + 2;
        if (!apply_warning_group_option(opts, name, true)) {
            driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                           "unknown warning option '%s'", arg);
        }
        return true;
    }
    return false;
}

void options::usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [options] <input>... [-o <output>]\n"
        "\n"
        "X Tools C Compiler (xcc) — C11 compiler driver for Z80\n"
        "\n"
        "options:\n"
        "  -o <file>         Output file\n"
        "  -c                Compile and assemble only, emit .rel\n"
        "  -S                Compile only, emit assembly\n"
        "  --c1mode          Read preprocessed C from stdin and emit assembly\n"
        "  -c1-mode          Alias for --c1mode\n"
        "  -O0               No optimization (default)\n"
        "  -O1               Late target cleanup only (peephole + tiny backend fusions)\n"
        "  -O2               Smart optimizer baseline (IR + backend + O1 cleanup)\n"
        "  -Of               O2-based speed profile with validated speed hooks\n"
        "  -O3               Separately routed experimental speed profile\n"
        "  -Os               Size-biased smart optimization\n"
        "  --opt-code-size   Alias for -Os (SDCC compatibility)\n"
        "  --opt-code-speed  Alias for -Of (SDCC compatibility)\n"
        "  -f<name>          Enable one optimization family\n"
        "  -fno-<name>       Disable one optimization family\n"
        "  -w                Disable all warnings\n"
        "  -W0..-W3          Warning levels (none, default, Wall, Wall+extra)\n"
        "  -Wall, -Wextra    Enable grouped warnings\n"
        "  -Wpedantic        Enable pedantic warnings\n"
        "  -W<name>          Enable one warning group\n"
        "  -Wno-<name>       Disable one warning group\n"
        "  -Werror[=<name>]  Promote warnings to errors\n"
        "  -Wno-error[=<name>]\n"
        "                    Stop promoting warnings to errors\n"
        "  -mz80             Accepted for SDCC compatibility\n"
        "  -I<dir>           Add include directory\n"
        "  -D<macro>[=val]   Define preprocessor macro\n"
        "  --nostdinc        Do not add the default target include directory\n"
        "  -std=c11          Language standard (only c11 supported)\n"
        "  -masm <dialect>   Assembler dialect: sdasz80 (default) or gnuas\n"
        "  -masm=<dialect>   Assembler dialect: sdasz80 (default) or gnuas\n"
        "  --platform <name> Select target platform include defaults\n"
        "  --platform=<name> Select target platform include defaults\n"
        "  --float-format <fmt>\n"
        "                    Float ABI: ieee32, ieee16, fixed8_8, fixed16_16, fixed24_8\n"
        "  --float-format=<fmt>\n"
        "                    Float ABI: ieee32, ieee16, fixed8_8, fixed16_16, fixed24_8\n"
        "  --sdcccall <0|1>  SDCC-compatible default ABI selector\n"
        "  -fsigned-char     Make plain 'char' signed\n"
        "  -funsigned-char   Make plain 'char' unsigned (default)\n"
        "  -g                Emit debug info\n"
        "  --dump-ir         Dump lowered IR to stderr\n"
        "  --mode=sdcc       Output for SDCC sdasz80 assembler (default)\n"
        "  --mode=gnu        Output for GNU binutils assembler\n"
        "  -L<dir>, -l<name> Forwarded to the linker\n"
        "  -B <prefix>       Forwarded to the linker runtime/toolchain search path\n"
        "  -nostdlib         Forwarded to the linker\n"
        "  -nostartfiles     Forwarded to the linker\n"
        "  --no-default-runtime\n"
        "                    Forwarded to the linker\n"
        "  --oformat=<fmt>   Forwarded to the linker (xl, binary, ihx; elf reserved)\n"
        "  -T*, --script=<file>, --section-start=<name>=<addr>\n"
        "                    Forwarded to the linker\n"
        "  --binary-range=<lo>-<hi>, --reserve=<lo>-<hi>\n"
        "                    Forwarded to the linker\n"
        "  -e <sym>, -Map=<file>, -M\n"
        "                    Forwarded to the linker\n"
        "  -Wl,<args>        Forward comma-separated args to the linker\n"
        "  -v                Verbose output\n"
        "  --version         Print version\n"
        "  -h, --help        Print this help\n",
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
            fprintf(stdout, "xcc %s (X Tools C Compiler for Z80)\n", XCC_VERSION);
            exit(0);
        }
        if (strcmp(a, "-S") == 0) {
            opts.mode = output_mode::ASSEMBLY;
        } else if (strcmp(a, "-c") == 0) {
            opts.mode = output_mode::OBJECT;
        } else if (strcmp(a, "--c1mode") == 0
                   || strcmp(a, "--c1-mode") == 0
                   || strcmp(a, "-c1-mode") == 0) {
            opts.c1_mode = true;
        } else if (strcmp(a, "--opt-code-size") == 0) {
            opts.opt = opt_level::Os;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
        } else if (strcmp(a, "--opt-code-speed") == 0) {
            opts.opt = opt_level::Of;
            opts.opt_settings = optimization_settings::for_level(opts.opt);
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
        } else if (apply_warning_option(opts, a)) {
            // Handled above so -Wl,<...> remains a linker option.
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
        } else if (strcmp(a, "-fsigned-char") == 0) {
            opts.plain_char_unsigned = false;
        } else if (strcmp(a, "-funsigned-char") == 0) {
            opts.plain_char_unsigned = true;
        } else if (strncmp(a, "-fno-", 5) == 0) {
            const char *name = a + 5;
            if (!apply_opt_flag(opts, name, false))
                driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                               "unknown optimization flag '%s'", a);
        } else if (strncmp(a, "-f", 2) == 0 && a[2] != '\0') {
            const char *name = a + 2;
            if (!apply_opt_flag(opts, name, true))
                driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                               "unknown optimization flag '%s'", a);
        } else if (strcmp(a, "-mz80") == 0) {
            // Accepted for SDCC/z88dk driver compatibility; xcc only targets Z80.
        } else if (strncmp(a, "-I", 2) == 0) {
            opts.include_paths.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strncmp(a, "-D", 2) == 0) {
            opts.defines.push_back(a[2] != '\0' ? a + 2 : (i + 1 < argc ? argv[++i] : ""));
        } else if (strcmp(a, "--nostdinc") == 0) {
            opts.use_default_include_paths = false;
        } else if (strcmp(a, "--platform") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: --platform requires a value\n");
                exit(1);
            }
            opts.platform_name = normalize_target_name(argv[++i]);
        } else if (strncmp(a, "--platform=", 11) == 0) {
            opts.platform_name = normalize_target_name(a + 11);
        } else if (strcmp(a, "--float-format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: --float-format requires a value\n");
                exit(1);
            }
            const char *format = argv[++i];
            if (!apply_float_format_option(opts, format)) {
                fprintf(stderr, "xcc: error: unknown float format '%s'\n", format);
                exit(1);
            }
        } else if (strncmp(a, "--float-format=", 15) == 0) {
            const char *format = a + 15;
            if (!apply_float_format_option(opts, format)) {
                fprintf(stderr, "xcc: error: unknown float format '%s'\n", format);
                exit(1);
            }
        } else if (strcmp(a, "--sdcccall") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: --sdcccall requires a value\n");
                exit(1);
            }
            call_abi abi = call_abi::DEFAULT;
            const char *mode = argv[++i];
            if (!parse_default_call_mode(mode, abi)) {
                fprintf(stderr, "xcc: error: --sdcccall value must be 0 or 1\n");
                exit(1);
            }
            opts.default_call_abi = abi;
        } else if (strncmp(a, "--sdcccall=", 11) == 0) {
            fprintf(stderr, "xcc: error: use '--sdcccall 0' or '--sdcccall 1'\n");
            exit(1);
        } else if (strcmp(a, "--call-mode") == 0
                   || strncmp(a, "--call-mode=", 12) == 0) {
            fprintf(stderr,
                    "xcc: error: unsupported option '--call-mode'; "
                    "use '--sdcccall 0' or '--sdcccall 1'\n");
            exit(1);
        } else if (strcmp(a, "--std-c89") == 0
                   || strcmp(a, "--std-sdcc89") == 0
                   || strcmp(a, "--std-c95") == 0
                   || strcmp(a, "--std-c99") == 0
                   || strcmp(a, "--std-sdcc99") == 0
                   || strcmp(a, "--std-c11") == 0
                   || strcmp(a, "--std-sdcc11") == 0
                   || strcmp(a, "--std-c2x") == 0
                   || strcmp(a, "--std-sdcc2x") == 0) {
            // Accepted for SDCC compatibility; xcc always parses as hosted C11/C23-ish xcc mode.
        } else if (strncmp(a, "-std=", 5) == 0) {
            // We only support c11; silently accept c99/c11/gnu11 etc.
        } else if (strncmp(a, "-masm=", 6) == 0) {
            const char *mode = a + 6;
            if (!apply_asm_dialect_option(opts, mode))
                driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                               "unknown assembler dialect '%s'", mode);
        } else if (strcmp(a, "-masm") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xcc: error: -masm requires a dialect\n");
                exit(1);
            }
            const char *mode = argv[++i];
            if (!apply_asm_dialect_option(opts, mode))
                driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                               "unknown assembler dialect '%s'", mode);
        } else if (strncmp(a, "--mode=", 7) == 0) {
            const char *mode = a + 7;
            if (!apply_asm_dialect_option(opts, mode))
                driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                               "unknown mode '%s'", mode);
        } else if (strcmp(a, "-g") == 0) {
            opts.debug = true;
        } else if (strcmp(a, "--dump-ir") == 0) {
            opts.dump_ir = true;
        } else if (strcmp(a, "-v") == 0) {
            opts.verbose = true;
        } else if (a[0] == '-') {
            // Unknown flag — warn but continue (gcc-compatible)
            driver_warning(opts, warning_group::UNKNOWN_WARNING_OPTION,
                           "unrecognized option '%s'", a);
        } else {
            opts.input_files.push_back(a);
        }
    }

    if (opts.input_files.empty() && !opts.c1_mode) {
        fprintf(stderr, "xcc: error: no input files\n");
        exit(1);
    }

    if (opts.use_default_include_paths)
        add_default_include_paths(opts, argv[0]);
    add_float_format_defines(opts);
    add_default_call_mode_defines(opts);
    add_plain_char_defines(opts);
    if (!opts.platform_name.empty()) {
        opts.defines.push_back(platform_define_name(opts.platform_name));
    }
    opts.driver_dir = resolve_process_executable(argv[0]).parent_path().string();

    return opts;
}

} // namespace xcc
