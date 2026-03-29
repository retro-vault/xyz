//
// Combined Scanner+Lexer for Z80 source files.
// Converts a raw assembly string into a flat token stream,
// classifying mnemonics, registers, conditions, immediates,
// operators, directives, punctuation, and labels.
//
// Copyright (c) 2025 Tomaz Stih, all rights reserved
// License: MIT
//
// tstih
//
#include <cctype>

#include "token.h"
#include "lexer.h"
#include "z80.h"

namespace xas
{

    lexer::lexer(const std::string &src)
        : source(src)
    {
    }

    std::vector<token> lexer::lex()
    {
        tokens.clear();
        while (!is_at_end())
        {
            skip_whitespace_and_comments();
            if (is_at_end())
                break;
            scan_token();
        }
        tokens.emplace_back(token_type::END_OF_FILE, "");
        return tokens;
    }

    // Low‑level helpers
    char lexer::advance() { return source[src_pos++]; }
    char lexer::peek() const { return src_pos < source.size() ? source[src_pos] : '\0'; }
    char lexer::peek_next() const { return (src_pos + 1 < source.size()) ? source[src_pos + 1] : '\0'; }
    bool lexer::is_at_end() const { return src_pos >= source.size(); }

    void lexer::skip_whitespace_and_comments()
    {
        while (!is_at_end())
        {
            char c = peek();
            if (std::isspace(c))
            {
                advance();
            }
            else if (c == ';')
            {
                // skip to end of line
                while (!is_at_end() && peek() != '\n')
                    advance();
            }
            else
            {
                break;
            }
        }
    }

    // Main dispatch
    void lexer::scan_token()
    {
        char c = advance();

        if (is_identifier_start(c))
        {
            tokens.push_back(scan_identifier());
            return;
        }
        if (std::isdigit(c) || (c == '0' && (peek() == 'x' || peek() == 'X')) || c == '$')
        {
            tokens.push_back(scan_number_or_hex());
            return;
        }
        // + / - can be part of indexed tokens or operators
        if ((c == '+' || c == '-'))
        {
            tokens.push_back(scan_operator_or_punct(c));
            return;
        }
        // punctuation
        switch (c)
        {
        case ':':
            tokens.emplace_back(token_type::COLON, ":");
            return;
        case ',':
            tokens.emplace_back(token_type::COMMA, ",");
            return;
        case '(':
            tokens.emplace_back(token_type::LBRACKET, "(");
            return;
        case ')':
            tokens.emplace_back(token_type::RBRACKET, ")");
            return;
        default:
            tokens.emplace_back(token_type::UNKNOWN, std::string(1, c));
            return;
        }
    }

    // Identifier: mnemonic, register, condition, directive or label
    bool lexer::is_identifier_start(char c) const
    {
        return std::isalpha(c) || c == '_';
    }

    bool lexer::is_identifier_part(char c) const
    {
        return std::isalnum(c) || c == '_';
    }

    token lexer::scan_identifier()
    {
        size_t start = src_pos - 1;
        while (!is_at_end() && is_identifier_part(peek()))
            advance();
        std::string text = source.substr(start, src_pos - start);

        // Check directive first
        auto &dirs = get_mnemonic_tokens(); // directives share token_type via your maps
        auto dit = dirs.find(text);
        if (dit != dirs.end() && token_category(dit->second) == CAT_DIRECTIVE)
            return token(dit->second, text);

        // Mnemonic
        if (dit != dirs.end() && token_category(dit->second) == CAT_MNEMONIC)
            return token(dit->second, text);

        // Register
        auto &rt = get_register_tokens();
        auto rit = rt.find(text);
        if (rit != rt.end())
            return token(rit->second, text);

        // Condition
        auto &ct = get_condition_tokens();
        auto cit = ct.find(text);
        if (cit != ct.end())
            return token(cit->second, text);

        // Directives
        if (text == "ORG")
            return token(token_type::DIRECTIVE_ORG, text);
        if (text == "DB")
            return token(token_type::DIRECTIVE_DB, text);

        // Otherwise it's an identifier/label
        return token(token_type::IDENTIFIER, text);
    }

    // Number or hex literal
    token lexer::scan_number_or_hex()
    {
        size_t start = src_pos - 1;

        // 0xNN
        if (source[start] == '0' && (peek() == 'x' || peek() == 'X'))
        {
            advance(); // x
            while (!is_at_end() && std::isxdigit(peek()))
                advance();
        }
        // $NN or $+NN or $-NN
        else if (source[start] == '$')
        {
            if (peek() == '+' || peek() == '-')
                advance();
            while (!is_at_end() && std::isxdigit(peek()))
                advance();
        }
        // trailing h/H hex
        else
        {
            while (!is_at_end() && (std::isxdigit(peek()) || peek() == 'h' || peek() == 'H'))
                advance();
        }

        return token(token_type::IMMEDIATE_VALUE,
                     source.substr(start, src_pos - start));
    }

    // Operator (for indexed offsets) or punctuation fallback
    token lexer::scan_operator_or_punct(char c)
    {
        // In practice '+' or '-' only appears within () for IX+N
        return token(token_type::OPERATOR, std::string(1, c));
    }

} // namespace xas