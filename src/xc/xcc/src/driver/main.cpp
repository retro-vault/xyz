//
// main.cpp — xcc compiler driver.
//
// Entry point for the xcc compiler.  Parses command-line options,
// then for each input file: reads it, runs the lexer, parser, IR
// generator, and Z80 code generator in sequence, optionally applies
// the peephole optimizer, and writes the output assembly file.
//
// The pipeline is intentionally linear with no shared state between
// files, so multiple input files are just processed in a loop.
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
#include "backend/z80/z80peep.h"
#include "backend/z80/debug_info.h"
#include <xbfd/xbfd.h>

#include <cstdlib>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>
#include <unordered_map>

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
    case xbfd::calling_convention::xcc_z88dk_fastcall: return call_abi::Z88DK_FASTCALL;
    case xbfd::calling_convention::xcc_z88dk_callee:   return call_abi::Z88DK_CALLEE;
    case xbfd::calling_convention::xcc_naked:          return call_abi::NAKED;
    case xbfd::calling_convention::xcc_interrupt:      return call_abi::INTERRUPT;
    case xbfd::calling_convention::xcc_critical:       return call_abi::CRITICAL;
    default:                                           return call_abi::DEFAULT;
    }
}

static bool is_source_input(const std::string &path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = path.substr(dot);
    for (auto &ch : ext)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return ext == ".c";
}

static bool is_metadata_input(const std::string &path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = path.substr(dot);
    for (auto &ch : ext)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
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
    if (!f) {
        fprintf(stderr, "xcc: error: cannot open '%s'\n", path.c_str());
        exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ----- Derive output filename ----------------------------------------
static std::string derive_output(const std::string &input, output_mode mode) {
    // Strip extension and add appropriate one
    std::string base = input;
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    (void)mode;
    return base + ".s";
}

// ----- Compile one file ----------------------------------------------
static int compile_file_to_text(const std::string &input_path,
                                const options &opts,
                                const std::string &out_path,
                                std::string &asm_text,
                                const std::unordered_map<std::string, call_abi> *imported_abis = nullptr) {
    if (opts.verbose)
        fprintf(stderr, "xcc: compiling %s\n", input_path.c_str());

    std::string raw = read_file(input_path);

    // ----- 0. Preprocess ---------------------------------------------
    diag_engine diag;
    preprocessor pp(diag, opts.include_paths, opts.defines);
    std::string src = pp.process(raw, input_path);

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
    sema sema_pass(diag, imported_abis);
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
    if (opts.opt_settings.peephole) {
        asm_text = z80_peep::optimize(asm_text);
    }

    return 0;
}

// ----- Compile one file ----------------------------------------------
static int compile_file(const std::string &input_path,
                        const options &opts,
                        const std::unordered_map<std::string, call_abi> *imported_abis = nullptr) {
    std::string out_path = opts.output_file.empty()
                           ? derive_output(input_path, opts.mode)
                           : opts.output_file;
    std::string asm_text;
    int rc = compile_file_to_text(input_path, opts, out_path, asm_text, imported_abis);
    if (rc != 0)
        return rc;

    // ----- 6. Write output -------------------------------------------
    if (out_path == "-") {
        // Write to stdout
        fputs(asm_text.c_str(), stdout);
    } else {
        std::ofstream out(out_path);
        if (!out) {
            fprintf(stderr, "xcc: error: cannot write '%s'\n", out_path.c_str());
            return 1;
        }
        out << asm_text;
        if (opts.verbose)
            fprintf(stderr, "xcc: wrote %s\n", out_path.c_str());
    }

    return 0;
}

} // namespace xcc

// ----- main ----------------------------------------------------------
int main(int argc, char **argv) {
    xcc::options opts = xcc::options::parse(argc, argv);

    std::vector<std::string> source_inputs;
    std::unordered_map<std::string, xcc::abi_import_record> imported_records;
    for (const auto &input : opts.input_files) {
        if (xcc::is_source_input(input)) {
            source_inputs.push_back(input);
            continue;
        }
        if (xcc::is_metadata_input(input)) {
            try {
                xcc::import_abi_metadata(input, imported_records);
            } catch (const std::exception &e) {
                fprintf(stderr, "xcc: warning: failed to import ABI metadata from '%s': %s\n",
                        input.c_str(), e.what());
            }
            continue;
        }
        source_inputs.push_back(input);
    }

    if (source_inputs.empty()) {
        fprintf(stderr, "xcc: error: no C source input files\n");
        return 1;
    }

    std::unordered_map<std::string, xcc::call_abi> imported_abis;
    for (const auto &[name, record] : imported_records)
        imported_abis.emplace(name, record.abi);

    int status = 0;
    for (auto &input : source_inputs) {
        int r = 1;
        try {
            r = xcc::compile_file(input, opts,
                                  imported_abis.empty() ? nullptr : &imported_abis);
        } catch (const xcc::fatal_error &) {
            // fatal diagnostic already printed by diag_engine::fatal()
        } catch (const std::exception &e) {
            fprintf(stderr, "xcc: error: %s\n", e.what());
        }
        if (r != 0) status = r;
    }
    return status;
}
