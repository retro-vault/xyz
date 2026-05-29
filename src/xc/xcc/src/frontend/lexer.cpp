//
// lexer.cpp — C11 lexer implementation.
//
// Tokenises a complete in-memory source string character by character.
// Handles all C11 tokens: keywords, identifiers, integer/float/char/
// string literals, and all operators.  Preprocessor lines (# …) are
// skipped entirely; an external cpp is expected to have already run.
// Unicode string/char prefixes (L, u, U, u8) are consumed and the
// content is returned as a plain narrow token.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/lexer.h"
#include <cassert>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace xcc {

// ----- token helpers -------------------------------------------------

bool token::is_type_specifier() const {
    switch (kind) {
    case tk::KW_VOID: case tk::KW_CHAR: case tk::KW_SHORT:
    case tk::KW_INT:  case tk::KW_LONG: case tk::KW_FLOAT:
    case tk::KW_DOUBLE: case tk::KW_SIGNED: case tk::KW_UNSIGNED:
    case tk::KW__BOOL:  case tk::KW__COMPLEX: case tk::KW__IMAGINARY:
    case tk::KW_STRUCT: case tk::KW_UNION:  case tk::KW_ENUM:
        return true;
    default:
        return false;
    }
}

bool token::is_type_qualifier() const {
    return kind == tk::KW_CONST || kind == tk::KW_VOLATILE ||
           kind == tk::KW_RESTRICT || kind == tk::KW__ATOMIC;
}

bool token::is_storage_class() const {
    switch (kind) {
    case tk::KW_AUTO: case tk::KW_EXTERN: case tk::KW_REGISTER:
    case tk::KW_STATIC: case tk::KW_TYPEDEF: case tk::KW__THREAD_LOCAL:
        return true;
    default:
        return false;
    }
}

const char *token::kind_name() const {
    switch (kind) {
#define CASE(k) case tk::k: return #k
    CASE(INT_LIT); CASE(FLOAT_LIT); CASE(CHAR_LIT); CASE(STR_LIT);
    CASE(IDENT);
    CASE(KW_AUTO); CASE(KW_EXTERN); CASE(KW_REGISTER); CASE(KW_STATIC); CASE(KW_TYPEDEF);
    CASE(KW_VOID); CASE(KW_CHAR); CASE(KW_SHORT); CASE(KW_INT); CASE(KW_LONG);
    CASE(KW_FLOAT); CASE(KW_DOUBLE); CASE(KW_SIGNED); CASE(KW_UNSIGNED);
    CASE(KW__BOOL); CASE(KW__COMPLEX); CASE(KW__IMAGINARY); CASE(KW_STRUCT); CASE(KW_UNION); CASE(KW_ENUM);
    CASE(KW_CONST); CASE(KW_VOLATILE); CASE(KW_RESTRICT);
    CASE(KW_INLINE); CASE(KW__ALIGNAS); CASE(KW__ALIGNOF);
    CASE(KW__ATOMIC); CASE(KW__NORETURN); CASE(KW__THREAD_LOCAL);
    CASE(KW__STATIC_ASSERT); CASE(KW__GENERIC);
    CASE(KW_BREAK); CASE(KW_CASE); CASE(KW_CONTINUE); CASE(KW_DEFAULT);
    CASE(KW_DO); CASE(KW_ELSE); CASE(KW_FOR); CASE(KW_GOTO); CASE(KW_IF);
    CASE(KW_RETURN); CASE(KW_SWITCH); CASE(KW_WHILE); CASE(KW_SIZEOF);
    CASE(KW___TYPEOF__); CASE(KW___TYPES_COMPAT_P); CASE(KW___BIT_CAST);
    CASE(LBRACE); CASE(RBRACE); CASE(LBRACKET); CASE(RBRACKET);
    CASE(LPAREN); CASE(RPAREN); CASE(SEMICOLON); CASE(COLON); CASE(COMMA);
    CASE(DOT); CASE(ARROW); CASE(ELLIPSIS); CASE(QUESTION);
    CASE(EQ); CASE(PLUS_EQ); CASE(MINUS_EQ); CASE(STAR_EQ); CASE(SLASH_EQ);
    CASE(PERCENT_EQ); CASE(AMP_EQ); CASE(PIPE_EQ); CASE(CARET_EQ);
    CASE(LSHIFT_EQ); CASE(RSHIFT_EQ);
    CASE(EQ_EQ); CASE(BANG_EQ); CASE(LT); CASE(GT); CASE(LT_EQ); CASE(GT_EQ);
    CASE(AMP_AMP); CASE(PIPE_PIPE); CASE(BANG);
    CASE(AMP); CASE(PIPE); CASE(CARET); CASE(TILDE); CASE(LSHIFT); CASE(RSHIFT);
    CASE(PLUS); CASE(MINUS); CASE(STAR); CASE(SLASH); CASE(PERCENT);
    CASE(PLUS_PLUS); CASE(MINUS_MINUS);
    CASE(LATTR); CASE(RATTR);
    CASE(HASH); CASE(END_OF_FILE); CASE(ERROR);
#undef CASE
    default: return "?";
    }
}

// ----- Keyword table -------------------------------------------------

tk lexer::keyword_kind(const std::string &s) {
    static const std::unordered_map<std::string, tk> kw = {
        {"auto",           tk::KW_AUTO},
        {"extern",         tk::KW_EXTERN},
        {"register",       tk::KW_REGISTER},
        {"static",         tk::KW_STATIC},
        {"typedef",        tk::KW_TYPEDEF},
        {"void",           tk::KW_VOID},
        {"char",           tk::KW_CHAR},
        {"short",          tk::KW_SHORT},
        {"int",            tk::KW_INT},
        {"long",           tk::KW_LONG},
        {"float",          tk::KW_FLOAT},
        {"double",         tk::KW_DOUBLE},
        {"signed",         tk::KW_SIGNED},
        {"unsigned",       tk::KW_UNSIGNED},
        {"_Bool",          tk::KW__BOOL},
        {"_Complex",       tk::KW__COMPLEX},
        {"_Imaginary",     tk::KW__IMAGINARY},
        {"struct",         tk::KW_STRUCT},
        {"union",          tk::KW_UNION},
        {"enum",           tk::KW_ENUM},
        {"const",          tk::KW_CONST},
        {"volatile",       tk::KW_VOLATILE},
        {"restrict",       tk::KW_RESTRICT},
        {"inline",         tk::KW_INLINE},
        {"_Alignas",       tk::KW__ALIGNAS},
        {"_Alignof",       tk::KW__ALIGNOF},
        {"_Atomic",        tk::KW__ATOMIC},
        {"_Noreturn",      tk::KW__NORETURN},
        {"_Thread_local",  tk::KW__THREAD_LOCAL},
        {"_Static_assert", tk::KW__STATIC_ASSERT},
        {"_Generic",       tk::KW__GENERIC},
        {"break",          tk::KW_BREAK},
        {"case",           tk::KW_CASE},
        {"continue",       tk::KW_CONTINUE},
        {"default",        tk::KW_DEFAULT},
        {"do",             tk::KW_DO},
        {"else",           tk::KW_ELSE},
        {"for",            tk::KW_FOR},
        {"goto",           tk::KW_GOTO},
        {"if",             tk::KW_IF},
        {"return",         tk::KW_RETURN},
        {"switch",         tk::KW_SWITCH},
        {"while",          tk::KW_WHILE},
        {"sizeof",         tk::KW_SIZEOF},
        {"__asm__",        tk::KW___ASM__},
        {"__asm",          tk::KW___ASM__},
        {"__attribute__",  tk::KW___ATTRIBUTE__},
        {"__extension__",  tk::KW___EXTENSION__},
        {"_Pragma",        tk::KW__PRAGMA},
        // GNU keyword aliases
        {"__volatile__",   tk::KW_VOLATILE},
        {"__const__",      tk::KW_CONST},
        {"__signed__",     tk::KW_SIGNED},
        {"__inline__",     tk::KW_INLINE},
        {"__inline",       tk::KW_INLINE},
        {"__restrict__",   tk::KW_RESTRICT},
        {"__typeof__",     tk::KW___TYPEOF__},
        {"__typeof",       tk::KW___TYPEOF__},
        {"typeof",         tk::KW___TYPEOF__},
        {"__builtin_types_compatible_p", tk::KW___TYPES_COMPAT_P},
        {"__builtin_bit_cast",           tk::KW___BIT_CAST},
    };
    auto it = kw.find(s);
    return it != kw.end() ? it->second : tk::IDENT;
}

// ----- lexer core ----------------------------------------------------

lexer::lexer(diag_engine &diag, std::string src, const char *filename)
    : src_(std::move(src)), file_(filename), diag_(diag) {}

char lexer::cur() const {
    return pos_ < src_.size() ? src_[pos_] : '\0';
}

char lexer::peek_char(int offset) const {
    size_t p = pos_ + offset;
    return p < src_.size() ? src_[p] : '\0';
}

char lexer::advance() {
    char c = cur();
    ++pos_;
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

source_loc lexer::make_loc() const { return {file_, line_, col_}; }

token lexer::make(tk k, std::string text, source_loc loc) const {
    token t;
    t.kind = k;
    t.loc  = loc;
    t.text = std::move(text);
    return t;
}

token lexer::error_tok(source_loc loc, const char *msg) {
    diag_.error(loc, "%s", msg);
    token t;
    t.kind = tk::ERROR;
    t.loc  = loc;
    t.text = "";
    return t;
}

void lexer::skip_whitespace_and_comments() {
    while (pos_ < src_.size()) {
        char c = cur();
        if (std::isspace((unsigned char)c)) {
            advance();
        } else if (c == '/' && peek_char() == '/') {
            // Line comment
            while (pos_ < src_.size() && cur() != '\n')
                advance();
        } else if (c == '/' && peek_char() == '*') {
            advance(); advance(); // consume /*
            while (pos_ + 1 < src_.size()) {
                if (cur() == '*' && peek_char() == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

token lexer::lex_ident_or_keyword() {
    auto loc = make_loc();
    std::string s;
    while (pos_ < src_.size() && (std::isalnum((unsigned char)cur()) || cur() == '_' || cur() == '$'))
        s += advance();
    tk k = keyword_kind(s);
    return make(k, s, loc);
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

token lexer::lex_number() {
    auto loc = make_loc();
    std::string s;
    bool is_float = false;

    if (cur() == '0' && (peek_char() == 'x' || peek_char() == 'X')) {
        // Hex
        s += advance(); s += advance(); // 0x
        while (hex_digit(cur()) >= 0) s += advance();
        if (cur() == '.') {               // hex fractional part
            is_float = true;
            s += advance();
            while (hex_digit(cur()) >= 0) s += advance();
        }
        if (cur() == 'p' || cur() == 'P') {
            is_float = true;
            s += advance();
            if (cur() == '+' || cur() == '-') s += advance();
            while (std::isdigit((unsigned char)cur())) s += advance();
        }
    } else {
        while (std::isdigit((unsigned char)cur())) s += advance();
        if (cur() == '.') { is_float = true; s += advance(); }
        while (std::isdigit((unsigned char)cur())) s += advance();
        if (cur() == 'e' || cur() == 'E') {
            is_float = true;
            s += advance();
            if (cur() == '+' || cur() == '-') s += advance();
            while (std::isdigit((unsigned char)cur())) s += advance();
        }
    }

    // Suffixes (u, l, ul, ll, f, etc.)
    std::string suffix;
    while (std::isalpha((unsigned char)cur())) suffix += advance();

    token t = make(is_float ? tk::FLOAT_LIT : tk::INT_LIT, s + suffix, loc);
    if (is_float) {
        t.fval = std::strtod(s.c_str(), nullptr);
    } else {
        // Use stoull to handle large unsigned literals like 0xffffffffffffffff
        // without throwing out_of_range, then reinterpret as signed.
        try {
            t.ival = (long long)std::stoull(s, nullptr, 0);
        } catch (...) {
            t.ival = 0;
        }
    }
    return t;
}

static char decode_escape(char c) {
    switch (c) {
    case 'a':  return '\a';
    case 'b':  return '\b';
    case 'f':  return '\f';
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case 'v':  return '\v';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"':  return '"';
    case '0':  return '\0';
    default:   return c;
    }
}

// Called immediately after consuming the leading '\'.
// Reads and consumes the rest of the escape sequence, returns decoded byte.
char lexer::lex_escape() {
    char c = advance();
    if (c == 'x' || c == 'X') {
        // Hex escape: \xNN
        int val = 0;
        while (hex_digit(cur()) >= 0)
            val = val * 16 + hex_digit(advance());
        return static_cast<char>(val & 0xFF);
    }
    if (c >= '1' && c <= '7') {
        // Octal escape: \NNN (1-3 digits, first already in c)
        int val = c - '0';
        for (int i = 0; i < 2 && cur() >= '0' && cur() <= '7'; ++i)
            val = val * 8 + (advance() - '0');
        return static_cast<char>(val & 0xFF);
    }
    if (c == 'u') {
        // Unicode \uXXXX — narrow to low 8 bits
        int val = 0;
        for (int i = 0; i < 4 && hex_digit(cur()) >= 0; ++i)
            val = val * 16 + hex_digit(advance());
        return static_cast<char>(val & 0xFF);
    }
    if (c == 'U') {
        // Unicode \UXXXXXXXX — narrow to low 8 bits
        int val = 0;
        for (int i = 0; i < 8 && hex_digit(cur()) >= 0; ++i)
            val = val * 16 + hex_digit(advance());
        return static_cast<char>(val & 0xFF);
    }
    return decode_escape(c);
}

token lexer::lex_char_literal() {
    auto loc = make_loc();
    advance(); // consume '
    int64_t val = 0;
    if (cur() == '\\') {
        advance();
        val = (unsigned char)lex_escape();
    } else {
        val = (unsigned char)advance();
    }
    if (cur() != '\'') return error_tok(loc, "unterminated character literal");
    advance();
    // C: plain char is signed on this target; sign-extend values in 0x80..0xFF.
    if (val >= 0x80 && val <= 0xFF)
        val = (int64_t)(int8_t)(uint8_t)val;
    token t = make(tk::CHAR_LIT, "", loc);
    t.ival = val;
    return t;
}

token lexer::lex_string_literal(int char_width) {
    auto loc = make_loc();
    advance(); // consume "
    std::string raw, decoded;
    while (pos_ < src_.size() && cur() != '"' && cur() != '\n') {
        if (cur() == '\\') {
            advance();
            char c = lex_escape();
            raw += '\\'; decoded += c;
        } else {
            char c = advance();
            raw += c; decoded += c;
        }
    }
    if (cur() != '"') return error_tok(loc, "unterminated string literal");
    advance();
    token t = make(tk::STR_LIT, raw, loc);
    t.sval = decoded;
    t.ival = char_width;
    return t;
}

token lexer::lex_operator() {
    auto loc = make_loc();
    char c = advance();
    char n = cur();

#define TWO(a,b,k) if (c==(a) && n==(b)) { advance(); return make(tk::k, {a,b}, loc); }
#define THREE(a,b,d,k) if (c==(a) && n==(b) && peek_char()==(d)) { advance(); advance(); return make(tk::k, {a,b,d}, loc); }

    switch (c) {
    case '{': return make(tk::LBRACE,    "{", loc);
    case '}': return make(tk::RBRACE,    "}", loc);
    case '[':
        if (n == '[') { advance(); return make(tk::LATTR,   "[[", loc); }
        return make(tk::LBRACKET,  "[", loc);
    case ']':
        if (n == ']') { advance(); return make(tk::RATTR,   "]]", loc); }
        return make(tk::RBRACKET,  "]", loc);
    case '(': return make(tk::LPAREN,    "(", loc);
    case ')': return make(tk::RPAREN,    ")", loc);
    case ';': return make(tk::SEMICOLON, ";", loc);
    case ':': if (n == '>') { advance(); return make(tk::RBRACKET, ":>", loc); } // digraph
              return make(tk::COLON,     ":", loc);
    case ',': return make(tk::COMMA,     ",", loc);
    case '?': return make(tk::QUESTION,  "?", loc);
    case '~': return make(tk::TILDE,     "~", loc);
    case '#': return make(tk::HASH,      "#", loc);

    case '=': TWO('=','=',EQ_EQ);   return make(tk::EQ,       "=",  loc);
    case '!': TWO('!','=',BANG_EQ); return make(tk::BANG,     "!",  loc);
    case '+': TWO('+','=',PLUS_EQ); TWO('+','+',PLUS_PLUS); return make(tk::PLUS,  "+", loc);
    case '-': TWO('-','=',MINUS_EQ); TWO('-','-',MINUS_MINUS);
              TWO('-','>',ARROW);   return make(tk::MINUS, "-", loc);
    case '*': TWO('*','=',STAR_EQ);    return make(tk::STAR,  "*",  loc);
    case '/': TWO('/','=',SLASH_EQ);   return make(tk::SLASH, "/",  loc);
    case '%': TWO('%','=',PERCENT_EQ);
              if (n == '>') { advance(); return make(tk::RBRACE,   "%>", loc); } // digraph
              return make(tk::PERCENT,"%", loc);
    case '&': TWO('&','=',AMP_EQ); TWO('&','&',AMP_AMP); return make(tk::AMP,   "&", loc);
    case '|': TWO('|','=',PIPE_EQ); TWO('|','|',PIPE_PIPE); return make(tk::PIPE, "|", loc);
    case '^': TWO('^','=',CARET_EQ);   return make(tk::CARET, "^",  loc);

    case '<':
        if (n == '<') {
            advance();
            if (cur() == '=') { advance(); return make(tk::LSHIFT_EQ, "<<=", loc); }
            return make(tk::LSHIFT, "<<", loc);
        }
        if (n == ':') { advance(); return make(tk::LBRACKET, "<:", loc); } // digraph
        if (n == '%') { advance(); return make(tk::LBRACE,   "<%", loc); } // digraph
        TWO('<','=',LT_EQ);
        return make(tk::LT, "<", loc);

    case '>':
        if (n == '>') {
            advance();
            if (cur() == '=') { advance(); return make(tk::RSHIFT_EQ, ">>=", loc); }
            return make(tk::RSHIFT, ">>", loc);
        }
        TWO('>','=',GT_EQ);
        return make(tk::GT, ">", loc);

    case '.':
        if (n == '.' && peek_char() == '.') {
            advance(); advance(); return make(tk::ELLIPSIS, "...", loc);
        }
        return make(tk::DOT, ".", loc);

    default:
        break;
    }
#undef TWO
#undef THREE
    char buf[64];
    snprintf(buf, sizeof(buf), "unexpected character '%c' (0x%02x)", c, (unsigned char)c);
    return error_tok(loc, buf);
}

token lexer::lex_one() {
    skip_whitespace_and_comments();
    if (pos_ >= src_.size()) return make(tk::END_OF_FILE, "", make_loc());

    char c = cur();

    // Preprocessor line markers: # N "filename" or # line N "filename"
    if (c == '#') {
        std::string directive;
        while (pos_ < src_.size() && cur() != '\n')
            directive += advance();
        const char *p = directive.c_str();
        while (*p == ' ' || *p == '\t') ++p;
        if (strncmp(p, "line", 4) == 0 && (p[4] == ' ' || p[4] == '\t'))
            p += 4;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p >= '1' && *p <= '9') {
            int lineno = 0;
            while (*p >= '0' && *p <= '9') lineno = lineno * 10 + (*p++ - '0');
            line_ = lineno;
            col_  = 1;
            while (*p == ' ' || *p == '\t') ++p;
            if (*p == '"') {
                ++p;
                std::string fname;
                while (*p && *p != '"') fname += *p++;
                if (!fname.empty()) {
                    file_strings_.push_back(fname);
                    file_ = file_strings_.back().c_str();
                }
            }
        }
        return lex_one();
    }

    if (c == '\'' ) return lex_char_literal();
    if (c == '"'  ) return lex_string_literal();

    // Unicode/wide string/char literal prefixes: u8"", L"", u"", U"", L'', u'', U''
    if (c == 'u' && peek_char() == '8' && peek_char(2) == '"') {
        advance(); advance(); return lex_string_literal(1);
    }
    if ((c == 'L') && peek_char() == '"') { advance(); return lex_string_literal(2); }
    if ((c == 'u') && peek_char() == '"') { advance(); return lex_string_literal(2); }
    if ((c == 'U') && peek_char() == '"') { advance(); return lex_string_literal(4); }
    if ((c == 'L' || c == 'u' || c == 'U') && peek_char() == '\'') {
        advance(); return lex_char_literal();
    }

    if (std::isalpha((unsigned char)c) || c == '_' || c == '$') return lex_ident_or_keyword();
    if (std::isdigit((unsigned char)c) ||
        (c == '.' && std::isdigit((unsigned char)peek_char())))
        return lex_number();

    return lex_operator();
}

token lexer::next() {
    if (has_lookahead_) {
        has_lookahead_ = false;
        token t = lookahead_;
        // Shift second lookahead down if present.
        if (has_lookahead2_) {
            lookahead_      = lookahead2_;
            has_lookahead_  = true;
            has_lookahead2_ = false;
        }
        return t;
    }
    return lex_one();
}

const token &lexer::peek() {
    if (!has_lookahead_) {
        lookahead_     = lex_one();
        has_lookahead_ = true;
    }
    return lookahead_;
}

const token &lexer::peek2() {
    // Ensure both slots are filled.
    peek(); // fill lookahead_ if needed
    if (!has_lookahead2_) {
        lookahead2_     = lex_one();
        has_lookahead2_ = true;
    }
    return lookahead2_;
}

token lexer::expect(tk k) {
    token t = next();
    if (t.kind != k) {
        char buf[128];
        token exp{}; exp.kind = k;
        snprintf(buf, sizeof(buf), "expected '%s', got '%s'",
                 exp.kind_name(), t.kind_name());
        return error_tok(t.loc, buf);
    }
    return t;
}

} // namespace xcc
