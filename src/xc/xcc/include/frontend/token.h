//
// token.h — C11 token definitions for the xcc lexer.
//
// Defines the source_loc struct (file/line/col), the tk enum class
// covering every C11 keyword, operator, and literal kind, and the
// token struct that carries a single lexed token with its raw text,
// decoded numeric/string value, and source position.
//
// The token struct also exposes small predicate helpers
// (is_type_specifier, is_type_qualifier, is_storage_class) that the
// parser calls when deciding whether the current token starts a
// declaration.  Keeping them here avoids a parser-to-lexer dependency.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include <cstdint>
#include <string>

namespace xcc {

// ----- source location -----------------------------------------------

struct source_loc {
    const char *file = "<unknown>";
    int         line = 1;
    int         col  = 1;
};

// ----- token kind ----------------------------------------------------
//
// Each enumerator maps to exactly one lexical token class.  Keywords
// are clustered so that range comparisons work for the predicate
// helpers on token (is_keyword(), is_type_specifier(), etc.).

enum class tk {
    // Literals
    INT_LIT, FLOAT_LIT, CHAR_LIT, STR_LIT,

    // Identifier / keywords
    IDENT,
    // Storage class
    KW_AUTO, KW_EXTERN, KW_REGISTER, KW_STATIC, KW_TYPEDEF,
    // Type specifiers
    KW_VOID, KW_CHAR, KW_SHORT, KW_INT, KW_LONG,
    KW_FLOAT, KW_DOUBLE, KW_SIGNED, KW_UNSIGNED,
    KW__BOOL, KW__COMPLEX, KW__IMAGINARY,
    KW_STRUCT, KW_UNION, KW_ENUM,
    // Type qualifiers
    KW_CONST, KW_VOLATILE, KW_RESTRICT,
    // Function specifier
    KW_INLINE,
    // C11 alignment
    KW__ALIGNAS, KW__ALIGNOF,
    // C11 atomics / thread / noreturn
    KW__ATOMIC, KW__NORETURN, KW__THREAD_LOCAL,
    KW__STATIC_ASSERT, KW__GENERIC,
    // Statements
    KW_BREAK, KW_CASE, KW_CONTINUE, KW_DEFAULT,
    KW_DO, KW_ELSE, KW_FOR, KW_GOTO, KW_IF,
    KW_RETURN, KW_SWITCH, KW_WHILE,
    KW_SIZEOF,
    KW___ASM__,         // __asm__("...") / __asm("...") — GNU inline assembly
    KW___ATTRIBUTE__,   // __attribute__((...)) — GNU attribute, parsed and ignored
    KW___EXTENSION__,   // __extension__ — GNU extension suppressor, no-op
    KW__PRAGMA,         // _Pragma("...") — C11 pragma operator, no-op
    KW___TYPEOF__,      // __typeof__(expr) / __typeof__(type) — GNU typeof
    KW___TYPES_COMPAT_P,// __builtin_types_compatible_p(t1, t2) — GNU type compat check
    KW___BIT_CAST,      // __builtin_bit_cast(type, expr) — C23 bit reinterpretation

    // C23 keywords
    KW_BOOL,            // bool  — first-class type keyword (C23; was a macro in C11)
    KW_TRUE,            // true  — boolean literal
    KW_FALSE,           // false — boolean literal
    KW_NULLPTR,         // nullptr — null pointer constant
    KW_CONSTEXPR,       // constexpr — compile-time constant object
    KW_TYPEOF_UNQUAL,   // typeof_unqual — typeof stripped of qualifiers
    KW__BITINT,         // _BitInt(N) — bit-precise integer type
    KW_CHAR8_T,         // char8_t — distinct unsigned char type for UTF-8

    // Punctuation / operators
    LBRACE, RBRACE,         // { }
    LBRACKET, RBRACKET,     // [ ]
    LPAREN, RPAREN,         // ( )
    SEMICOLON,              // ;
    COLON,                  // :
    COMMA,                  // ,
    DOT,                    // .
    ARROW,                  // ->
    ELLIPSIS,               // ...
    QUESTION,               // ?

    // Assignment operators
    EQ,                     // =
    PLUS_EQ, MINUS_EQ,      // += -=
    STAR_EQ, SLASH_EQ,      // *= /=
    PERCENT_EQ,             // %=
    AMP_EQ, PIPE_EQ,        // &= |=
    CARET_EQ,               // ^=
    LSHIFT_EQ, RSHIFT_EQ,   // <<= >>=

    // Comparison
    EQ_EQ, BANG_EQ,         // == !=
    LT, GT, LT_EQ, GT_EQ,  // < > <= >=

    // Logical
    AMP_AMP, PIPE_PIPE,     // && ||
    BANG,                   // !

    // Bitwise
    AMP, PIPE, CARET, TILDE, // & | ^ ~
    LSHIFT, RSHIFT,          // << >>

    // Arithmetic
    PLUS, MINUS, STAR, SLASH, PERCENT, // + - * / %
    PLUS_PLUS, MINUS_MINUS,            // ++ --

    // C23 attribute brackets
    LATTR,                  // [[
    RATTR,                  // ]]

    // Special
    HASH,                   // # (preprocessor line, skipped by lexer)
    END_OF_FILE,
    ERROR
};

// ----- token ---------------------------------------------------------
//
// One lexed token.  All fields are valid regardless of kind, but only
// the relevant ones are populated by the lexer:
//   text   — always set to the raw source text of the token
//   ival   — set for INT_LIT and CHAR_LIT
//   fval   — set for FLOAT_LIT
//   sval   — set for STR_LIT (decoded bytes, no NUL terminator)

struct token {
    tk          kind  = tk::ERROR;
    source_loc  loc;
    std::string text;
    int64_t     ival  = 0;
    double      fval  = 0.0;
    std::string sval;
    int         char_width = 1; // 1=char, 2=char16_t/wchar_t, 4=char32_t, 8=char8_t

    //
    // Return true if this token's kind equals k.
    //
    bool is(tk k) const { return kind == k; }

    //
    // Return true if this token is any keyword (KW_AUTO … KW_SIZEOF).
    //
    bool is_keyword() const {
        return kind >= tk::KW_AUTO && kind <= tk::KW_SIZEOF;
    }

    //
    // Return true if this token can begin a type-specifier in a
    // declaration (void, char, short, int, long, float, double,
    // signed, unsigned, _Bool, _Complex, struct, union, enum).
    //
    bool is_type_specifier() const;

    //
    // Return true if this token is a type-qualifier keyword
    // (const, volatile, restrict, _Atomic).
    //
    bool is_type_qualifier() const;

    //
    // Return true if this token is a storage-class keyword
    // (auto, extern, register, static, typedef).
    //
    bool is_storage_class() const;

    //
    // Return a human-readable name for the token kind.
    // Useful for error messages.  The returned pointer is static.
    //
    const char *kind_name() const;
};

} // namespace xcc
