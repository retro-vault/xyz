// lexer.h
//
// xas lexer — converts a source stream into a flat token list.
// Mode-independent: the same lexer handles both SDCC and GNU syntax;
// directive names are distinguished later by the parser.
//
// Numeric literal formats recognised:
//   0x1A  0X1A   — hexadecimal
//   0b101 0B101  — binary
//   0777         — octal (leading zero)
//   255          — decimal
//   1Ah  1AH     — SDCC hex suffix
//
// Comments: ; to end of line (both modes)
//           // to end of line (GNU mode)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_FRONTEND_LEXER_HPP
#define XAS_FRONTEND_LEXER_HPP

#include <istream>
#include <string>
#include <vector>

#include <xas/frontend/token.h>

namespace xas {

    class lexer {
    public:
        //
        // Tokenise the entire stream.  source_name is used in error messages.
        // Throws lex_error on unrecognised input.
        //
        std::vector<token> tokenise(const std::string& source_name,
                                    std::istream& in);

    private:
        std::string  file_;
        int          line_ = 1;
        std::string  src_;
        size_t       pos_  = 0;

        char peek(size_t ahead = 0) const;
        char advance();
        void skip_whitespace_inline();
        void skip_to_eol();

        token next_token();
        token read_comment();
        token read_integer();
        token read_string();
        token read_ident_or_directive();
    };

} // namespace xas

#endif // XAS_FRONTEND_LEXER_HPP
