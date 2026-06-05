// token.h
//
// Token types and the token struct used by the xas lexer.  A token
// carries its kind, raw text, source file name, and line number.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_FRONTEND_TOKEN_HPP
#define XAS_FRONTEND_TOKEN_HPP

#include <cstdint>
#include <string>

namespace xas {

    enum class token_kind {
        // Literals
        ident,          // identifier or mnemonic
        integer,        // 0x1A, 0b1010, 0777, 255
        string_lit,     // "text"
        char_lit,       // 'A'

        // Punctuation
        colon,          // :
        comma,          // ,
        lparen,         // (
        rparen,         // )
        plus,           // +
        minus,          // -
        star,           // *
        slash,          // /
        percent,        // %
        tilde,          // ~
        amp,            // &
        pipe,           // |
        caret,          // ^
        lshift,         // <<
        rshift,         // >>
        hash,           // # (immediate prefix in some dialects)
        at,             // @ (GNU immediate prefix)
        dollar,         // $ (current address)
        dot,            // . (current address, GNU style)
        newline,        // end of logical line
        directive,      // .keyword  (starts with .)
        eq,             // = (assignment / EQU form)

        // Control
        eof,
        unknown
    };

    struct token {
        token_kind  kind   = token_kind::unknown;
        std::string text;           // uppercased for ident (mnemonic compare), lowercased for directive
        std::string raw;            // original case — use for symbol/label names
        int64_t     int_val = 0;    // filled for token_kind::integer
        std::string file;
        int         line   = 0;
    };

} // namespace xas

#endif // XAS_FRONTEND_TOKEN_HPP
