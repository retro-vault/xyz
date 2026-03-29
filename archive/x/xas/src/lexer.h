//
// Combined Scanner+Lexer for Z80 source files.
// Converts a raw assembly string into a flat token stream,
// classifying mnemonics, registers, conditions, immediates,
// operators, directives, punctuation, and labels.
//
// USAGE:
//   lexer lex(source_string);
//   auto tokens = lex.lex();  // yields vector<token>, ending with END_OF_FILE
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#pragma once

#include <string>
#include <vector>

#include "token.h" // defines token_type and token

namespace xas
{

    /// A single‑pass Z80 lexer that emits tokens ready for parsing.
    class lexer
    {
    public:
        /// Construct a lexer over the given source text.
        explicit lexer(const std::string &source);

        /// Perform tokenization and return the token stream.
        /// The returned vector is always terminated by an END_OF_FILE token.
        std::vector<token> lex();

    private:
        const std::string source;
        size_t src_pos = 0;
        std::vector<token> tokens;

        // Character‑level utilities
        char advance();
        char peek() const;
        char peek_next() const;
        bool is_at_end() const;
        void skip_whitespace_and_comments();

        // Token‑level scanning
        void scan_token();
        token scan_identifier();
        token scan_number_or_hex();
        token scan_operator_or_punct(char c);

        // Helpers for classification
        bool is_identifier_start(char c) const;
        bool is_identifier_part(char c) const;
    };

} // namespace xas