// assembler.h
//
// Two‑pass Z80 assembler: parses tokens into machine code and symbol table.
//
// USAGE:
//   lexer lex(src);
//   auto toks = lex.lex();
//   xas::assembler a(toks);
//   xas::obj out = a.assemble();
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#pragma once

#include "token.h"
#include "z80.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace xas
{

    /// In‑memory assembled object.
    struct obj
    {
        std::vector<uint8_t> code;                      ///< Raw bytes
        std::unordered_map<std::string, size_t> labels; ///< Label → offset
    };

    /// The assembler driver: two‑pass over a flat token stream.
    class assembler
    {
    public:
        /// Construct over a ready‑made token stream.
        explicit assembler(const std::vector<token> &tokens);

        /// Run both passes and return the assembled object.
        obj assemble();

    private:
        const std::vector<token> &toks;
        size_t index = 0;
        size_t location_counter = 0;
        obj result;

        // --- PASS 1: collect labels & compute sizes ---
        void first_pass();
        void record_label(const token &name);
        void skip_statement();

        // --- PASS 2: emit bytes & backpatch labels ---
        void second_pass();
        void assemble_statement();
        void assemble_directive(const token &dir);
        void assemble_instruction(const token &mnem);

        // Helpers
        bool is_label_decl() const;
        bool is_directive() const;
        bool is_mnemonic() const;
        token peek() const;
        token advance();
        bool is_at_end() const;
    };

} // namespace xas
