// test-lexer.cpp
#include <gtest/gtest.h>

#include "lexer.h"
#include "token.h"
#include "z80.h"

using namespace xas;

// Helper to strip lexemes for readability
static std::vector<token_type> types_of(const std::vector<token> &toks)
{
    std::vector<token_type> out;
    for (auto &t : toks)
        out.push_back(t.type);
    return out;
}

TEST(LexerTest, SimpleLoadAndHalt)
{
    std::string src = "LD A, 0xFF ; comment\nHALT";
    lexer lex(src);
    auto toks = lex.lex();

    // Expect: LD, A, ,, 0xFF, HALT, EOF
    std::vector<token_type> expected = {
        token_type::MNEMONIC_LD,
        token_type::REGISTER8_A,
        token_type::COMMA,
        token_type::IMMEDIATE_VALUE,
        token_type::MNEMONIC_HALT,
        token_type::END_OF_FILE};
    EXPECT_EQ(types_of(toks), expected);
    EXPECT_EQ(toks[0].lexeme, "LD");
    EXPECT_EQ(toks[1].lexeme, "A");
    EXPECT_EQ(toks[3].lexeme, "0xFF");
}

TEST(LexerTest, IndexedAddressing)
{
    std::string src = "LD HL,(IX+1)";
    lexer lex(src);
    auto toks = lex.lex();

    // Expect: LD, HL, ,, (, IX, +, 1, ), EOF
    std::vector<token_type> expected = {
        token_type::MNEMONIC_LD,
        token_type::REGISTER16_HL,
        token_type::COMMA,
        token_type::LBRACKET,
        token_type::REGISTER16_IX,
        token_type::OPERATOR,
        token_type::IMMEDIATE_VALUE,
        token_type::RBRACKET,
        token_type::END_OF_FILE};
    EXPECT_EQ(types_of(toks), expected);

    // check lexemes
    EXPECT_EQ(toks[3].lexeme, "(");
    EXPECT_EQ(toks[4].lexeme, "IX");
    EXPECT_EQ(toks[5].lexeme, "+");
    EXPECT_EQ(toks[6].lexeme, "1");
}

TEST(LexerTest, ConditionsAndDirectives)
{
    std::string src = "JP Z,Start\nORG 0x1000";
    lexer lex(src);
    auto toks = lex.lex();

    // Expect: JP, Z, ,, Start, JP ?, ORG, IMMEDIATE, EOF
    std::vector<token_type> expected = {
        token_type::MNEMONIC_JP,
        token_type::CONDITION_Z,
        token_type::COMMA,
        token_type::IDENTIFIER,
        token_type::DIRECTIVE_ORG,
        token_type::IMMEDIATE_VALUE,
        token_type::END_OF_FILE};
    // We merge Start label name and directive in a single stream,
    // skipping newlines and comments.
    // Note: label handling happens in parser/lexer stage, not here.
    EXPECT_EQ(types_of(toks), expected);
    EXPECT_EQ(toks[3].lexeme, "Start");
    EXPECT_EQ(toks[4].lexeme, "ORG");
    EXPECT_EQ(toks[5].lexeme, "0x1000");
}