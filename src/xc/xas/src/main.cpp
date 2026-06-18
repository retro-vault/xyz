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
#include <sstream>
#include <stdexcept>

#include <xas/cli.h>
#include <xas/errors.h>
#include <xas/backend/source_emitter.h>
#include <xas/backend/macro_translate.h>
#include <xas/frontend/lexer.h>
#include <xas/frontend/macro.h>
#include <xas/frontend/parser.h>
#include <xas/backend/emitter.h>

namespace xas {
    // Defined in codegen.cpp.
    void assemble(const stmt_list& stmts, emitter& emit,
                  const std::string& module_name,
                  const std::string& src_file,
                  asm_mode mode,
                  const std::vector<std::string>& defines);

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

        // Slurp the whole source.
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string source = buffer.str();

        const bool has_macros =
            xas::macro_processor::has_macro_directives(source);

        // Split source into physical lines (origin tracking for diagnostics).
        auto split_lines = [&](const std::string& s) {
            xas::src_lines lines;
            int lineno = 1;
            for (size_t i = 0; i < s.size(); ) {
                size_t nl = s.find('\n', i);
                std::string text = (nl == std::string::npos)
                                       ? s.substr(i) : s.substr(i, nl - i);
                lines.push_back({ text, opts.input, lineno++ });
                if (nl == std::string::npos) break;
                i = nl + 1;
            }
            return lines;
        };

        // --- Source-to-source conversion (--format) --------------------------
        if (opts.format.has_value()) {
            std::string document_text;
            if (has_macros) {
                // Translate compatible macros to the target dialect; expand the
                // rest so the converted output still assembles.
                xas::macro_translator tr(opts.mode, *opts.format);
                document_text = tr.translate(split_lines(source));
                std::ofstream out(opts.output);
                if (!out.is_open()) {
                    std::cerr << "xas: error: cannot create '" << opts.output << "'\n";
                    return 1;
                }
                out << document_text;
                return 0;
            }
            std::istringstream lex_in(source);
            xas::lexer lex;
            auto tokens = lex.tokenise(opts.input, lex_in);
            xas::parser par;
            auto stmts = par.parse(tokens, opts.mode);
            auto emit = xas::make_source_emitter(*opts.format);
            auto document = emit->emit(stmts);
            std::ofstream out(opts.output);
            if (!out.is_open()) {
                std::cerr << "xas: error: cannot create '" << opts.output << "'\n";
                return 1;
            }
            xas::write_formatted_document(out, document);
            return 0;
        }

        // --- Assembly path: fully expand macros first ------------------------
        if (has_macros) {
            xas::macro_processor mp(xas::make_macro_dialect(opts.mode));
            for (const std::string& d : opts.defines) {
                size_t eq = d.find('=');
                if (eq == std::string::npos) mp.add_define(d, "1");
                else mp.add_define(d.substr(0, eq), d.substr(eq + 1));
            }
            xas::src_lines expanded = mp.run(split_lines(source));
            std::string out;
            for (const xas::src_line& sl : expanded) { out += sl.text; out += '\n'; }
            source = std::move(out);
        }

        // Lex.
        std::istringstream lex_in(source);
        xas::lexer lex;
        auto tokens = lex.tokenise(opts.input, lex_in);

        // Parse.
        xas::parser par;
        auto stmts = par.parse(tokens, opts.mode);

        {
            // Choose emitter.
            std::unique_ptr<xas::emitter> emit;
            if (opts.mode == xas::asm_mode::gnu)
                emit = xas::make_elf_emitter(opts.output);
            else
                emit = xas::make_rel_emitter(opts.output);

            // Code generation (two-pass).
            std::string module = std::filesystem::path(opts.input).stem().string();
            xas::assemble(stmts, *emit, module, opts.input, opts.mode,
                          opts.defines);
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
