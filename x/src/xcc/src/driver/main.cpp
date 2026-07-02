//
// main.cpp — xcc compiler driver.
//
// Entry point for the xcc compiler.  Parses command-line options,
// then drives the toolchain like the GNU C driver: each .c input is
// compiled to assembly in-process, assembled by spawning xas, and the
// resulting objects are linked by spawning xld.  -S stops after
// compilation, -c stops after assembly, and the default mode links
// everything into an executable (a.out unless -o is given).
//
// The compile pipeline is intentionally linear with no shared state
// between files, so multiple input files are just processed in a loop.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "driver/options.h"
#include "frontend/diag.h"
#include "frontend/preproc.h"
#include "frontend/lexer.h"
#include "opt/iromod.h"
#include "opt/iropt.h"
#include "frontend/parser.h"
#include "frontend/sema.h"
#include "ir/irgen.h"
#include "backend/sdasz80_emitter.h"
#include "backend/gnuas_emitter.h"
#include "backend/z80/z80gen.h"
#include "backend/z80/debug_info.h"
#include <xopt/xopt.h>
#include <xbfd/xbfd.h>

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <exception>
#include <system_error>
#include <unordered_map>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace xcc {

struct abi_import_record {
    call_abi abi = call_abi::DEFAULT;
    std::string source;
};

static call_abi from_xbfd_calling_convention(xbfd::calling_convention cc) {
    switch (cc) {
    case xbfd::calling_convention::normal:             return call_abi::DEFAULT;
    case xbfd::calling_convention::xcc_sdcccall0:      return call_abi::SDCCCALL0;
    case xbfd::calling_convention::xcc_sdcccall1:      return call_abi::SDCCCALL1;
    case xbfd::calling_convention::xcc_z88dk_smallc:   return call_abi::Z88DK_SMALLC;
    case xbfd::calling_convention::xcc_z88dk_fastcall: return call_abi::Z88DK_FASTCALL;
    case xbfd::calling_convention::xcc_z88dk_callee:   return call_abi::Z88DK_CALLEE;
    case xbfd::calling_convention::xcc_naked:          return call_abi::NAKED;
    case xbfd::calling_convention::xcc_interrupt:      return call_abi::INTERRUPT;
    case xbfd::calling_convention::xcc_critical:       return call_abi::CRITICAL;
    default:                                           return call_abi::DEFAULT;
    }
}

// Input classification, GNU-driver style: C sources are compiled,
// assembly sources are assembled, everything else is handed to the
// linker untouched.
enum class input_kind {
    C_SOURCE,    // .c
    ASM_SOURCE,  // .s / .asm
    OBJECT,      // .rel / .lib / .o / .a / anything else
};

static std::string lower_extension(const std::string &path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos)
        return {};
    std::string ext = path.substr(dot);
    for (auto &ch : ext)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return ext;
}

static input_kind classify_input(const std::string &path) {
    const std::string ext = lower_extension(path);
    if (ext == ".c")
        return input_kind::C_SOURCE;
    if (ext == ".s" || ext == ".asm")
        return input_kind::ASM_SOURCE;
    return input_kind::OBJECT;
}

static bool is_metadata_input(const std::string &path) {
    const std::string ext = lower_extension(path);
    return ext == ".rel" || ext == ".lib" || ext == ".o" || ext == ".a";
}

static bool is_probable_function_symbol(const xbfd::object &obj,
                                        const xbfd::symbol &sym) {
    if (!sym.is_defined() || !sym.is_global() || sym.is_absolute())
        return false;
    if (sym.name.empty())
        return false;
    if (sym.name.rfind("G$", 0) == 0 || sym.name.rfind("XG$", 0) == 0
        || sym.name.rfind("C$", 0) == 0 || sym.name.rfind("A$", 0) == 0
        || sym.name.rfind("L", 0) == 0 || sym.name.rfind(".__", 0) == 0) {
        return false;
    }
    for (const auto &sec : obj.sections) {
        if (sec.name != sym.section_name)
            continue;
        return xbfd::has_flag(sec.flags, xbfd::section_flags::code)
            || sec.name == "_CODE" || sec.name == ".text" || sec.name == "_HOME";
    }
    return sym.section_name == "_CODE" || sym.section_name == ".text";
}

static std::string canonical_function_name(const std::string &name) {
    if (!name.empty() && name[0] == '_')
        return name.substr(1);
    return name;
}

static void add_imported_function_abi(
    std::unordered_map<std::string, abi_import_record> &imports,
    const std::string &name, call_abi abi, const std::string &source)
{
    if (name.empty() || abi == call_abi::DEFAULT)
        return;
    auto canonical = canonical_function_name(name);
    auto it = imports.find(canonical);
    if (it == imports.end()) {
        imports.emplace(std::move(canonical), abi_import_record{abi, source});
        return;
    }
    if (it->second.abi != abi) {
        fprintf(stderr,
                "xcc: warning: conflicting imported calling conventions for '%s' (%s vs %s)\n",
                canonical.c_str(), it->second.source.c_str(), source.c_str());
    }
}

static void import_abi_from_object(
    const xbfd::object &obj,
    const std::string &source_name,
    std::unordered_map<std::string, abi_import_record> &imports)
{
    for (const auto &fn : obj.debug.functions) {
        const auto abi = from_xbfd_calling_convention(fn.convention);
        add_imported_function_abi(imports, fn.name, abi, source_name);
    }

    if (obj.default_calling_convention == xbfd::calling_convention::unknown)
        return;
    const auto abi = from_xbfd_calling_convention(obj.default_calling_convention);
    for (const auto &sym : obj.symbols) {
        if (!is_probable_function_symbol(obj, sym))
            continue;
        add_imported_function_abi(imports, sym.name, abi, source_name);
    }
}

static void import_abi_metadata(
    const std::string &path,
    std::unordered_map<std::string, abi_import_record> &imports)
{
    auto obj = bfd::bfd::open_r(path);
    if (obj->check_format(bfd::format::archive)) {
        for (const auto &member : obj->members()) {
            if (member.data.has_value()) {
                std::istringstream input(*member.data);
                auto member_obj = bfd::bfd::open_r_stream(member.path, input);
                if (member_obj->check_format(bfd::format::object))
                    import_abi_from_object(member_obj->object(), member.path, imports);
                continue;
            }
            if (member.path.empty())
                continue;
            auto member_obj = bfd::bfd::open_r(member.path);
            if (member_obj->check_format(bfd::format::object))
                import_abi_from_object(member_obj->object(), member.path, imports);
        }
        return;
    }
    if (obj->check_format(bfd::format::object))
        import_abi_from_object(obj->object(), path, imports);
}

// ----- Read entire file into string ----------------------------------
static std::string read_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot open '" + path + "'");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string read_stdin() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

// ----- Derive output filename ----------------------------------------
static std::string strip_extension(const std::string &path) {
    std::string base = path;
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    return base;
}

static std::string derive_output(const std::string &input, output_mode mode) {
    return strip_extension(input)
         + (mode == output_mode::OBJECT ? ".rel" : ".s");
}

// ----- Compile one file ----------------------------------------------
static int compile_source_to_text(const std::string &input_path,
                                  const std::string &raw,
                                  bool run_preprocessor,
                                  const options &opts,
                                  const std::string &out_path,
                                  std::string &asm_text,
                                  const std::unordered_map<std::string, call_abi> *imported_abis = nullptr) {
    if (opts.verbose)
        fprintf(stderr, "xcc: compiling %s%s\n",
                input_path.c_str(),
                run_preprocessor ? "" : " from preprocessed stdin (--c1mode)");

    set_float_format(opts.float_fmt);
    set_default_call_abi(opts.default_call_abi);

    diag_engine diag;
    diag.set_options(opts.diagnostics);
    std::string src = raw;

    // ----- 0. Preprocess ---------------------------------------------
    if (run_preprocessor) {
        preprocessor pp(diag, opts.include_paths, opts.defines);
        src = pp.process(raw, input_path);
    }

    // ----- 1. Lex ----------------------------------------------------
    lexer lex(diag, src, input_path.c_str());

    // ----- 2. Parse --------------------------------------------------
    parser parser(lex);
    auto   tu = parser.parse();

    if (diag.has_errors()) {
        fprintf(stderr, "xcc: %d error(s) in '%s'\n",
                diag.error_count(), input_path.c_str());
        return 1;
    }

    // ----- 2.5 Semantic analysis (const enforcement etc.) ------------
    sema sema_pass(diag, imported_abis, opts.default_call_abi);
    sema_pass.check(*tu);
    if (diag.has_errors()) {
        fprintf(stderr, "xcc: %d error(s) in '%s'\n",
                diag.error_count(), input_path.c_str());
        return 1;
    }

    // ----- 3. Lower AST -> IR ----------------------------------------
    ir_gen irgen;
    auto  mod = irgen.lower(*tu);

    // ----- 3.5 IR optimization (-O2 / -O3 / -Os) ----------------------
    if (opts.opt_settings.has_module_passes()) {
        ir_module_optimizer::optimize(*mod, opts.opt_settings);
    }
    if (opts.opt_settings.has_function_ir_passes()) {
        for (auto &fn : mod->functions)
            ir_optimizer::optimize(fn, opts.opt_settings);
    }
    if (opts.dump_ir)
        mod->dump();

    // ----- 4. Code generation ----------------------------------------
    std::ostringstream asm_buf;
    {
        std::unique_ptr<asm_emitter> emitter;
        if (opts.dialect == asm_dialect::GNUAS)
            emitter = std::make_unique<gnuas_emitter>(asm_buf);
        else
            emitter = std::make_unique<sdasz80_emitter>(asm_buf);

        if (!run_preprocessor)
            emitter->comment("Compiled in c1-mode from preprocessed input");

        z80_gen codegen(*emitter);
        codegen.set_opt_settings(opts.opt_settings);
        if (opts.debug) {
            std::string base = out_path;
            auto dot = base.rfind('.');
            if (dot != std::string::npos) base = base.substr(0, dot);

            // Build the writer with the right format backend, then
            // attach an xcc adapter that translates IR types to xdi calls.
            auto writer = std::make_unique<xbfd::debug_writer>();
            if (opts.dialect == asm_dialect::GNUAS)
                writer->add_dwarf(asm_buf, input_path);
            else
                writer->add_sdcc(asm_buf, input_path, base + ".adb");

            codegen.set_debug(std::make_unique<xcc::xdi_adapter>(
                std::move(writer), input_path));
        }
        codegen.emit_module(*mod);
    }
    asm_text = asm_buf.str();

    // ----- 5. Peephole optimization ----------------------------------
    // Skip the assembly peephole optimizer for pathologically large output:
    // some layout-dependent rules (e.g. jp->jr) recompute the whole code
    // layout per application, which is O(n^2) and dominates compile time on
    // machine-generated stress inputs (e.g. the C23 translation-limit tests).
    // Generated code is slightly larger but correct.
    const size_t asm_line_count =
        static_cast<size_t>(std::count(asm_text.begin(), asm_text.end(), '\n'));
    constexpr size_t kMaxPeepholeLines = 8000;
    if (opts.opt_settings.peephole && asm_line_count <= kMaxPeepholeLines) {
        xopt::optimizer_options xopt_opts;
        switch (opts.opt_settings.level) {
        case opt_level::O0:
            xopt_opts.level = xopt::optimization_level::none;
            break;
        case opt_level::O1:
            xopt_opts.level = xopt::optimization_level::o1;
            break;
        case opt_level::O2:
            xopt_opts.level = xopt::optimization_level::o2;
            break;
        case opt_level::Os:
            xopt_opts.level = xopt::optimization_level::os;
            break;
        case opt_level::Of:
            xopt_opts.level = xopt::optimization_level::of;
            break;
        case opt_level::O3:
            xopt_opts.level = xopt::optimization_level::o3;
            break;
        }
        asm_text = xopt::optimize_assembly(asm_text, xopt_opts);
    }

    return 0;
}

static int compile_file_to_text(const std::string &input_path,
                                const options &opts,
                                const std::string &out_path,
                                std::string &asm_text,
                                const std::unordered_map<std::string, call_abi> *imported_abis = nullptr) {
    return compile_source_to_text(input_path, read_file(input_path), true,
                                  opts, out_path, asm_text, imported_abis);
}

static int compile_stdin_to_text(const std::string &logical_input_path,
                                 const options &opts,
                                 const std::string &out_path,
                                 std::string &asm_text,
                                 const std::unordered_map<std::string, call_abi> *imported_abis = nullptr) {
    return compile_source_to_text(logical_input_path, read_stdin(), false,
                                  opts, out_path, asm_text, imported_abis);
}

// ----- Write compiled assembly to a file or stdout --------------------
static int write_asm_output(const std::string &asm_text,
                            const std::string &out_path,
                            const options &opts) {
    if (out_path == "-") {
        fputs(asm_text.c_str(), stdout);
        return 0;
    }
    std::ofstream out(out_path);
    if (!out) {
        fprintf(stderr, "xcc: error: cannot write '%s'\n", out_path.c_str());
        return 1;
    }
    out << asm_text;
    if (opts.verbose)
        fprintf(stderr, "xcc: wrote %s\n", out_path.c_str());
    return 0;
}

// ----- Compile one file, with diagnostics caught ----------------------
// adb_base_path is only used to derive the .adb side-file name when -g
// is active; asm text is returned through asm_text.
static int compile_file_checked(const std::string &input_path,
                                const options &opts,
                                const std::string &adb_base_path,
                                std::string &asm_text,
                                const std::unordered_map<std::string, call_abi> *imported_abis) {
    try {
        return compile_file_to_text(input_path, opts, adb_base_path,
                                    asm_text, imported_abis);
    } catch (const fatal_error &) {
        // fatal diagnostic already printed by diag_engine::fatal()
    } catch (const std::exception &e) {
        fprintf(stderr, "xcc: error: %s\n", e.what());
    }
    return 1;
}

static int compile_stdin_checked(const std::string &logical_input_path,
                                 const options &opts,
                                 const std::string &adb_base_path,
                                 std::string &asm_text,
                                 const std::unordered_map<std::string, call_abi> *imported_abis) {
    try {
        return compile_stdin_to_text(logical_input_path, opts, adb_base_path,
                                     asm_text, imported_abis);
    } catch (const fatal_error &) {
        // fatal diagnostic already printed by diag_engine::fatal()
    } catch (const std::exception &e) {
        fprintf(stderr, "xcc: error: %s\n", e.what());
    }
    return 1;
}

// ----- Subprocess helpers ----------------------------------------------
static int run_tool(const std::vector<std::string> &args, bool verbose) {
    if (verbose) {
        std::string line;
        for (const auto &a : args) {
            if (!line.empty()) line += ' ';
            line += a;
        }
        fprintf(stderr, "xcc: %s\n", line.c_str());
    }

    std::vector<const char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args)
        argv.push_back(a.c_str());
    argv.push_back(nullptr);

#ifdef _WIN32
    const int status = _spawnvp(_P_WAIT, argv[0], argv.data());
    if (status == -1) {
        fprintf(stderr, "xcc: error: cannot execute '%s'\n", args[0].c_str());
        return 1;
    }
    return status;
#else
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "xcc: error: fork failed\n");
        return 1;
    }
    if (pid == 0) {
        execvp(argv[0], const_cast<char * const *>(argv.data()));
        fprintf(stderr, "xcc: error: cannot execute '%s'\n", args[0].c_str());
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "xcc: error: waitpid failed\n");
        return 1;
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    fprintf(stderr, "xcc: error: '%s' terminated abnormally\n", args[0].c_str());
    return 1;
#endif
}

// xas/xld live next to the xcc executable in an installed prefix;
// fall back to PATH lookup for development setups.
static std::string find_tool(const options &opts, const char *name) {
    if (!opts.driver_dir.empty()) {
        std::filesystem::path candidate =
            std::filesystem::path(opts.driver_dir) / name;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec))
            return candidate.string();
#ifdef _WIN32
        auto exe_candidate = candidate;
        exe_candidate += ".exe";
        ec.clear();
        if (std::filesystem::exists(exe_candidate, ec))
            return exe_candidate.string();
#endif
    }
    return name;
}

static const char *tool_mode_flag(const options &opts) {
    return opts.dialect == asm_dialect::GNUAS ? "--mode=gnu" : "--mode=sdcc";
}

static int assemble_file(const options &opts, const std::string &xas,
                         const std::string &src, const std::string &out) {
    std::vector<std::string> args = {xas, tool_mode_flag(opts)};
    if (opts.debug)
        args.push_back("-g");
    for (const auto &dir : opts.include_paths) {
        args.push_back("-I");
        args.push_back(dir);
    }
    for (const auto &def : opts.defines) {
        args.push_back("-D");
        args.push_back(def);
    }
    args.push_back(src);
    args.push_back("-o");
    args.push_back(out);
    return run_tool(args, opts.verbose);
}

static int link_files(const options &opts, const std::string &xld,
                      const std::vector<std::string> &inputs,
                      const std::string &out) {
    std::vector<std::string> args = {xld, tool_mode_flag(opts)};
    if (opts.debug)
        args.push_back("-g");
    if (!opts.platform_name.empty())
        args.push_back("--platform=" + opts.platform_name);
    for (const auto &arg : opts.linker_args)
        args.push_back(arg);
    if (opts.float_fmt != float_format::IEEE32)
        args.push_back("-lfixed");
    args.push_back("-o");
    args.push_back(out);
    for (const auto &input : inputs)
        args.push_back(input);
    return run_tool(args, opts.verbose);
}

// Scratch directory for intermediate .s/.rel files, removed on exit.
struct temp_dir {
    std::filesystem::path path;
    bool valid = false;

    bool create() {
        std::error_code ec;
        auto base = std::filesystem::temp_directory_path(ec);
        if (ec) {
            ec.clear();
            base = std::filesystem::current_path(ec);
        }
        if (ec) {
            fprintf(stderr, "xcc: error: cannot determine temporary directory\n");
            return false;
        }

        const auto stamp =
            static_cast<unsigned long long>(
                std::chrono::steady_clock::now().time_since_epoch().count());
        for (unsigned attempt = 0; attempt != 256; ++attempt) {
            auto candidate = base / ("xcc-" + std::to_string(stamp) + "-"
                                     + std::to_string(attempt));
            ec.clear();
            if (std::filesystem::create_directory(candidate, ec)) {
                path = std::move(candidate);
                valid = true;
                return true;
            }
        }

        if (ec) {
            fprintf(stderr, "xcc: error: cannot create temporary directory: %s\n",
                    ec.message().c_str());
        } else {
            fprintf(stderr, "xcc: error: cannot create temporary directory\n");
        }
        return false;
    }

    std::string make_name(int index, const std::string &input,
                          const char *ext) const {
        const auto stem = std::filesystem::path(input).stem().string();
        return (path / (std::to_string(index) + "-" + stem + ext)).string();
    }

    ~temp_dir() {
        if (valid) {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    }
};

} // namespace xcc

// ----- main ----------------------------------------------------------
int main(int argc, char **argv) {
    using namespace xcc;

    options opts = options::parse(argc, argv);

    if (opts.c1_mode) {
        std::string logical_input = "<stdin>";
        bool saw_logical_source = false;
        for (const auto &input : opts.input_files) {
            if (classify_input(input) != input_kind::C_SOURCE) {
                fprintf(stderr,
                        "xcc: error: input '%s' is incompatible with --c1mode; "
                        "read preprocessed C from stdin instead\n",
                        input.c_str());
                return 1;
            }
            if (!saw_logical_source) {
                logical_input = input;
                saw_logical_source = true;
            } else if (opts.verbose) {
                fprintf(stderr,
                        "xcc: ignoring extra c1-mode source hint '%s'\n",
                        input.c_str());
            }
        }

        std::string out = opts.output_file.empty() ? "-" : opts.output_file;
        std::string asm_text;
        int r = compile_stdin_checked(logical_input, opts, out, asm_text, nullptr);
        if (r == 0)
            r = write_asm_output(asm_text, out, opts);
        return r;
    }

    struct input_item {
        input_kind kind;
        std::string path;
    };
    std::vector<input_item> inputs;
    inputs.reserve(opts.input_files.size());
    for (const auto &input : opts.input_files)
        inputs.push_back({classify_input(input), input});

    // Import calling-convention metadata from object/archive inputs so
    // compiled calls match what the libraries were built with.
    std::unordered_map<std::string, abi_import_record> imported_records;
    for (const auto &item : inputs) {
        if (item.kind != input_kind::OBJECT || !is_metadata_input(item.path))
            continue;
        try {
            import_abi_metadata(item.path, imported_records);
        } catch (const std::exception &e) {
            fprintf(stderr, "xcc: warning: failed to import ABI metadata from '%s': %s\n",
                    item.path.c_str(), e.what());
        }
    }
    std::unordered_map<std::string, call_abi> imported_abis;
    for (const auto &[name, record] : imported_records)
        imported_abis.emplace(name, record.abi);
    const auto *abis = imported_abis.empty() ? nullptr : &imported_abis;

    // GNU semantics: -o with -c or -S only makes sense for one input.
    size_t compiled_count = 0;
    for (const auto &item : inputs) {
        if (item.kind == input_kind::C_SOURCE)
            ++compiled_count;
        else if (item.kind == input_kind::ASM_SOURCE
                 && opts.mode == output_mode::OBJECT)
            ++compiled_count;
    }
    if (opts.mode != output_mode::LINK && !opts.output_file.empty()
        && compiled_count > 1) {
        fprintf(stderr,
                "xcc: error: cannot specify -o with -c or -S with multiple files\n");
        return 1;
    }

    // ----- -S: compile only -------------------------------------------
    if (opts.mode == output_mode::ASSEMBLY) {
        if (compiled_count == 0) {
            fprintf(stderr, "xcc: error: no C source input files\n");
            return 1;
        }
        int status = 0;
        for (const auto &item : inputs) {
            if (item.kind != input_kind::C_SOURCE) {
                fprintf(stderr,
                        "xcc: warning: input '%s' unused because compilation stops at -S\n",
                        item.path.c_str());
                continue;
            }
            std::string out = opts.output_file.empty()
                              ? derive_output(item.path, opts.mode)
                              : opts.output_file;
            std::string asm_text;
            int r = compile_file_checked(item.path, opts, out, asm_text, abis);
            if (r == 0)
                r = write_asm_output(asm_text, out, opts);
            if (r != 0) status = r;
        }
        return status;
    }

    const std::string xas = find_tool(opts, "xas");

    // ----- -c: compile and assemble -------------------------------------
    if (opts.mode == output_mode::OBJECT) {
        if (compiled_count == 0) {
            fprintf(stderr, "xcc: error: no input files to assemble\n");
            return 1;
        }
        temp_dir scratch;
        int status = 0;
        int index = 0;
        for (const auto &item : inputs) {
            ++index;
            if (item.kind == input_kind::OBJECT) {
                fprintf(stderr,
                        "xcc: warning: input '%s' unused because linking is not done\n",
                        item.path.c_str());
                continue;
            }
            std::string out = opts.output_file.empty()
                              ? derive_output(item.path, opts.mode)
                              : opts.output_file;
            if (item.kind == input_kind::ASM_SOURCE) {
                int r = assemble_file(opts, xas, item.path, out);
                if (r != 0) status = r;
                continue;
            }
            if (!scratch.valid && !scratch.create())
                return 1;
            // Anchor the -g .adb side file next to the final .rel.
            std::string tmp_s = scratch.make_name(index, item.path, ".s");
            std::string asm_text;
            int r = compile_file_checked(item.path, opts, out, asm_text, abis);
            if (r == 0)
                r = write_asm_output(asm_text, tmp_s, opts);
            if (r == 0)
                r = assemble_file(opts, xas, tmp_s, out);
            if (r != 0) status = r;
        }
        return status;
    }

    // ----- default: compile, assemble, and link -------------------------
    temp_dir scratch;
    std::vector<std::string> link_inputs;
    link_inputs.reserve(inputs.size());
    int index = 0;
    for (const auto &item : inputs) {
        ++index;
        if (item.kind == input_kind::OBJECT) {
            link_inputs.push_back(item.path);
            continue;
        }
        if (!scratch.valid && !scratch.create())
            return 1;
        std::string src_s = item.path;
        if (item.kind == input_kind::C_SOURCE) {
            // Keep .s/.adb/.rel side by side in the scratch dir so xld
            // picks up the debug records when -g is active.
            src_s = scratch.make_name(index, item.path, ".s");
            std::string asm_text;
            int r = compile_file_checked(item.path, opts, src_s, asm_text, abis);
            if (r == 0)
                r = write_asm_output(asm_text, src_s, opts);
            if (r != 0)
                return r;
        }
        std::string tmp_rel = scratch.make_name(index, item.path, ".rel");
        int r = assemble_file(opts, xas, src_s, tmp_rel);
        if (r != 0)
            return r;
        link_inputs.push_back(tmp_rel);
    }

    if (link_inputs.empty()) {
        fprintf(stderr, "xcc: error: no input files\n");
        return 1;
    }

    const std::string xld = find_tool(opts, "xld");
    std::string out = opts.output_file.empty() ? "a.out" : opts.output_file;
    return link_files(opts, xld, link_inputs, out);
}
