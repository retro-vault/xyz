//
// lexer.h — hand-written C11 lexer for the xcc compiler.
//
// The lexer reads a complete source string and produces a stream of
// token values on demand.  It maintains a one-token lookahead so the
// parser can inspect the next token without consuming it.
//
// Unicode/wide literal prefixes (L, u, U, u8) are silently stripped;
// the string or character content is returned as a plain narrow token.
// Lines beginning with # are skipped entirely (external preprocessor
// assumed).
//
// Error handling: malformed tokens produce a tk::ERROR token and
// increment the internal error counter.  The caller should stop
// compilation once error_count() exceeds a reasonable limit.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "token.h"
#include "frontend/diag.h"
#include <string>
#include <vector>

namespace xcc {

class lexer {
public:
    //
    // Construct a lexer for the given source string.
    // filename is used only for source_loc in emitted tokens.
    //
    explicit lexer(diag_engine &diag, std::string src,
                   const char *filename = "<input>");

    //
    // Consume and return the next token from the source.
    // Returns a tk::END_OF_FILE token when the source is exhausted.
    //
    token next();

    //
    // Return a reference to the next token without consuming it.
    // Multiple calls return the same token until next() is called.
    //
    const token &peek();

    //
    // Return a reference to the token after the next one (second lookahead)
    // without consuming either token.
    //
    const token &peek2();

    //
    // Consume the next token and return it.  If the token's kind does
    // not match k, emit an error message and return an error token.
    //
    token expect(tk k);

    //
    // Return true if the source position has reached the end of input.
    //
    bool at_end() const { return pos_ >= src_.size(); }

    //
    // Return the diag_engine used by this lexer.
    //
    diag_engine &diag() const { return diag_; }

private:
    std::string              src_;
    size_t                   pos_  = 0;
    int                      line_ = 1;
    int                      col_  = 1;
    const char              *file_;
    token                    lookahead_;
    bool                     has_lookahead_ = false;
    token                    lookahead2_;
    bool                     has_lookahead2_ = false;
    diag_engine             &diag_;
    std::vector<std::string> file_strings_; // persistent storage for #line filenames

    char cur()  const;
    char peek_char(int offset = 1) const;
    char advance();
    void skip_whitespace_and_comments();

    token lex_one();
    token lex_ident_or_keyword();
    token lex_number();
    char  lex_escape();
    token lex_char_literal(int char_width = 1);
    token lex_string_literal(int char_width = 1);
    token lex_operator();

    source_loc make_loc() const;
    token make(tk k, std::string text, source_loc loc) const;
    token error_tok(source_loc loc, const char *msg);

    static tk keyword_kind(const std::string &s);
};

} // namespace xcc
