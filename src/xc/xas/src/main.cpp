// main.cpp
//
// xas — Z80 assembler for the xyz toolchain.
// Supports SDCC sdasz80 syntax (--mode=sdcc) and GNU gas syntax (--mode=gnu).
// Outputs SDCC .rel or ELF32 z80-elf object files.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <xas/cli.h>
#include <xas/errors.h>
#include <xas/backend/source_emitter.h>
#include <xas/frontend/lexer.h>
#include <xas/frontend/parser.h>
#include <xas/backend/emitter.h>

namespace xas {
    // Defined in codegen.cpp.
    void assemble(const stmt_list& stmts, emitter& emit,
                  const std::string& module_name,
                  const std::string& src_file,
                  asm_mode mode);

    // Defined in rel_emitter.cpp / elf_emitter.cpp.
    std::unique_ptr<emitter> make_rel_emitter(const std::string& path);
    std::unique_ptr<emitter> make_elf_emitter(const std::string& path);
}

int main(int argc, char** argv)
{
    try {
        auto opts = xas::parse_args(argc, argv);

        // Open input.
        std::ifstream in(opts.input);
        if (!in.is_open()) {
            std::cerr << "xas: error: cannot open '" << opts.input << "'\n";
            return 1;
        }

        // Lex.
        xas::lexer lex;
        auto tokens = lex.tokenise(opts.input, in);

        // Parse.
        xas::parser par;
        auto stmts = par.parse(tokens, opts.mode);

        if (opts.format.has_value()) {
            auto emit = xas::make_source_emitter(*opts.format);
            auto document = emit->emit(stmts);
            std::ofstream out(opts.output);
            if (!out.is_open()) {
                std::cerr << "xas: error: cannot create '" << opts.output << "'\n";
                return 1;
            }
            xas::write_formatted_document(out, document);
        } else {
            // Choose emitter.
            std::unique_ptr<xas::emitter> emit;
            if (opts.mode == xas::asm_mode::gnu)
                emit = xas::make_elf_emitter(opts.output);
            else
                emit = xas::make_rel_emitter(opts.output);

            // Code generation (two-pass).
            std::string module = std::filesystem::path(opts.input).stem().string();
            xas::assemble(stmts, *emit, module, opts.input, opts.mode);
        }

        return 0;

    } catch (const xas::asm_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xas: error: " << e.what() << "\n";
        return 1;
    }
}
