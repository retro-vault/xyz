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
#include <fstream>
#include <sstream>
#include <string>

namespace xcc {

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
static int compile_file(const std::string &input_path, const options &opts) {
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
    sema sema_pass(diag);
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
    std::string out_path = opts.output_file.empty()
                           ? derive_output(input_path, opts.mode)
                           : opts.output_file;

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
    std::string asm_text = asm_buf.str();

    // ----- 5. Peephole optimization ----------------------------------
    if (opts.opt_settings.peephole) {
        asm_text = z80_peep::optimize(asm_text);
    }

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

    int status = 0;
    for (auto &input : opts.input_files) {
        int r = 1;
        try {
            r = xcc::compile_file(input, opts);
        } catch (const xcc::fatal_error &) {
            // fatal diagnostic already printed by diag_engine::fatal()
        }
        if (r != 0) status = r;
    }
    return status;
}
