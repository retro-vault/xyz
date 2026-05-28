// lexer.cpp
//
// xas lexer — tokenises Z80 assembly source.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <xas/errors.hpp>
#include <xas/frontend/lexer.hpp>

namespace xas {

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    char lexer::peek(size_t ahead) const
    {
        size_t idx = pos_ + ahead;
        return idx < src_.size() ? src_[idx] : '\0';
    }

    char lexer::advance()
    {
        char c = src_[pos_++];
        if (c == '\n') ++line_;
        return c;
    }

    void lexer::skip_whitespace_inline()
    {
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (c == ' ' || c == '\t' || c == '\r')
                ++pos_;
            else
                break;
        }
    }

    void lexer::skip_to_eol()
    {
        while (pos_ < src_.size() && src_[pos_] != '\n')
            ++pos_;
    }

    // -------------------------------------------------------------------------
    // Integer literal parsing
    // -------------------------------------------------------------------------

    token lexer::read_integer()
    {
        token t;
        t.kind = token_kind::integer;
        t.file = file_;
        t.line = line_;

        size_t start = pos_;
        int base = 10;

        // Detect base prefix.
        if (src_[pos_] == '0' && pos_ + 1 < src_.size()) {
            char next = src_[pos_ + 1];
            if (next == 'x' || next == 'X') { base = 16; pos_ += 2; }
            else if (next == 'b' || next == 'B') { base = 2;  pos_ += 2; }
            else if (std::isdigit(next))          { base = 8;  ++pos_; }
            else                                  { ++pos_; } // just '0'
        } else {
            ++pos_;
        }

        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (base == 16 && std::isxdigit(static_cast<unsigned char>(c)))
                ++pos_;
            else if (base == 2 && (c == '0' || c == '1'))
                ++pos_;
            else if (base == 8 && c >= '0' && c <= '7')
                ++pos_;
            else if (base == 10 && std::isdigit(static_cast<unsigned char>(c)))
                ++pos_;
            else
                break;
        }

        // SDCC-style hex suffix: digits followed by 'h' or 'H'.
        // e.g.  1Ah  FFh  (only if no 0x prefix was used)
        if (base == 10 && pos_ < src_.size()) {
            char s = src_[pos_];
            if (s == 'h' || s == 'H') {
                base = 16;
                ++pos_;
            }
        }

        t.text = src_.substr(start, pos_ - start);
        try {
            t.int_val = static_cast<int64_t>(
                std::stoull(
                    // strip suffix and prefix for conversion
                    [&]() -> std::string {
                        std::string raw = t.text;
                        if (!raw.empty() && (raw.back() == 'h' || raw.back() == 'H'))
                            raw.pop_back();
                        return raw;
                    }(),
                    nullptr, base));
        } catch (...) {
            throw lex_error(file_, line_, "malformed integer: " + t.text);
        }
        return t;
    }

    // -------------------------------------------------------------------------
    // String literal
    // -------------------------------------------------------------------------

    token lexer::read_string()
    {
        token t;
        t.kind = token_kind::string_lit;
        t.file = file_;
        t.line = line_;
        ++pos_; // skip opening '"'
        std::string s;
        while (pos_ < src_.size() && src_[pos_] != '"' && src_[pos_] != '\n') {
            if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
                ++pos_;
                char esc = src_[pos_++];
                switch (esc) {
                    case 'n':  s += '\n'; break;
                    case 't':  s += '\t'; break;
                    case 'r':  s += '\r'; break;
                    case '\\': s += '\\'; break;
                    case '"':  s += '"';  break;
                    case '0':  s += '\0'; break;
                    default:   s += esc;  break;
                }
            } else {
                s += src_[pos_++];
            }
        }
        if (pos_ < src_.size() && src_[pos_] == '"') ++pos_;
        t.text = s;
        return t;
    }

    // -------------------------------------------------------------------------
    // Identifier / directive
    // -------------------------------------------------------------------------

    token lexer::read_ident_or_directive()
    {
        token t;
        t.file = file_;
        t.line = line_;

        bool is_dir = (src_[pos_] == '.');
        size_t start = pos_;

        // Allow letters, digits, underscores, dots in identifiers.
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_'
                || c == '.' || c == '\'' || c == '!')
                ++pos_;
            else
                break;
        }

        t.text = src_.substr(start, pos_ - start);

        if (is_dir) {
            t.kind = token_kind::directive;
            // Strip leading dot from text for convenience.
            if (!t.text.empty() && t.text[0] == '.')
                t.text = t.text.substr(1);
            // Save original case in raw BEFORE lowercasing (needed for dot-labels).
            t.raw = t.text;
            // Lowercase for case-insensitive directive matching.
            for (auto& c : t.text)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else {
            // raw preserves original case for symbol/label names.
            t.raw  = t.text;
            // text is uppercased for case-insensitive mnemonic comparison.
            for (auto& c : t.text)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            t.kind = token_kind::ident;
        }

        return t;
    }

    // -------------------------------------------------------------------------
    // next_token
    // -------------------------------------------------------------------------

    token lexer::next_token()
    {
        skip_whitespace_inline();

        if (pos_ >= src_.size()) {
            token t; t.kind = token_kind::eof; t.file = file_; t.line = line_;
            return t;
        }

        char c = src_[pos_];

        // Comments.
        if (c == ';' || (c == '/' && peek(1) == '/')) {
            skip_to_eol();
            // Fall through to return newline below.
        }

        if (pos_ >= src_.size()) {
            token t; t.kind = token_kind::eof; t.file = file_; t.line = line_;
            return t;
        }

        c = src_[pos_];

        if (c == '\n') {
            token t; t.kind = token_kind::newline;
            t.file = file_; t.line = line_;
            t.text = "\n";
            ++pos_; ++line_;
            return t;
        }

        if (c == '"') return read_string();

        if (c == '\'') {
            token t; t.kind = token_kind::char_lit;
            t.file = file_; t.line = line_;
            ++pos_;
            if (pos_ < src_.size()) {
                char ch = src_[pos_++];
                if (ch == '\\' && pos_ < src_.size()) {
                    char esc = src_[pos_++];
                    switch (esc) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        default:  ch = esc;  break;
                    }
                }
                t.int_val = static_cast<int64_t>(
                    static_cast<unsigned char>(ch));
                t.text = std::string(1, ch);
            }
            if (pos_ < src_.size() && src_[pos_] == '\'') ++pos_;
            return t;
        }

        if (std::isdigit(static_cast<unsigned char>(c)))
            return read_integer();

        // SDCC hex: starts with letter + digit sequence ending in h/H
        // e.g. 0FFh handled above; ABCDh starts with a letter — detect here
        if (std::isalpha(static_cast<unsigned char>(c))) {
            size_t j = pos_;
            bool all_hex = true;
            bool ends_h  = false;
            while (j < src_.size() && (std::isalnum(static_cast<unsigned char>(src_[j]))
                                       || src_[j] == '_')) {
                if (!std::isxdigit(static_cast<unsigned char>(src_[j]))
                    && src_[j] != 'h' && src_[j] != 'H')
                    all_hex = false;
                if ((src_[j] == 'h' || src_[j] == 'H') && j == src_.size() - 1)
                    ends_h = true;
                ++j;
            }
            // Check if the last char is h/H and ALL PRECEDING chars are hex digits.
            // Require length >= 2 so lone 'h'/'H' (register names) aren't mistaken.
            if (all_hex && j - pos_ >= 2 && (src_[j-1] == 'h' || src_[j-1] == 'H')) {
                // SDCC hex literal like 0FFh — read as integer.
                size_t start = pos_;
                pos_ = j;
                std::string raw = src_.substr(start, j - start);
                raw.pop_back(); // remove trailing h/H
                token t; t.kind = token_kind::integer; t.file = file_; t.line = line_;
                t.text = src_.substr(start, j - start);
                try {
                    t.int_val = static_cast<int64_t>(std::stoull(raw, nullptr, 16));
                } catch (...) {
                    throw lex_error(file_, line_, "malformed hex literal: " + t.text);
                }
                (void)ends_h;
                return t;
            }
            return read_ident_or_directive();
        }

        if (c == '.' || c == '_') return read_ident_or_directive();
        if (c == '$' || c == '@') {
            token t; t.kind = (c == '$') ? token_kind::dollar : token_kind::at;
            t.text = std::string(1, c); t.file = file_; t.line = line_;
            ++pos_; return t;
        }

#define TOK1(ch, kind_) \
        if (c == (ch)) { \
            token t; t.kind = token_kind::kind_; t.text = std::string(1,c); \
            t.file = file_; t.line = line_; ++pos_; return t; }

        TOK1(':', colon)
        TOK1(',', comma)
        TOK1('(', lparen)
        TOK1(')', rparen)
        TOK1('+', plus)
        TOK1('-', minus)
        TOK1('*', star)
        TOK1('/', slash)
        TOK1('%', percent)
        TOK1('~', tilde)
        TOK1('&', amp)
        TOK1('|', pipe)
        TOK1('^', caret)
        TOK1('#', hash)
#undef TOK1

        if (c == '<' && peek(1) == '<') {
            token t; t.kind = token_kind::lshift; t.text = "<<";
            t.file = file_; t.line = line_; pos_ += 2; return t;
        }
        if (c == '>' && peek(1) == '>') {
            token t; t.kind = token_kind::rshift; t.text = ">>";
            t.file = file_; t.line = line_; pos_ += 2; return t;
        }

        // Unknown character — skip and warn.
        token t; t.kind = token_kind::unknown;
        t.text = std::string(1, c); t.file = file_; t.line = line_;
        ++pos_;
        return t;
    }

    // -------------------------------------------------------------------------
    // tokenise — public entry point
    // -------------------------------------------------------------------------

    std::vector<token> lexer::tokenise(const std::string& source_name,
                                        std::istream& in)
    {
        file_ = source_name;
        line_ = 1;
        src_.assign((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
        pos_ = 0;

        std::vector<token> result;
        while (true) {
            token t = next_token();
            if (t.kind == token_kind::eof) {
                result.push_back(t);
                break;
            }
            // Collapse multiple newlines.
            if (t.kind == token_kind::newline && !result.empty()
                && result.back().kind == token_kind::newline)
                continue;
            result.push_back(std::move(t));
        }
        return result;
    }

} // namespace xas
