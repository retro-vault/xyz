// test-assembler.cpp
#include <gtest/gtest.h>

#include "lexer.h"
#include "assembler.h"
#include "token.h"

using namespace xas;

// Helper to lex and assemble a source string in one call
static obj assemble_source(const std::string &src)
{
    lexer lex(src);
    auto toks = lex.lex();
    assembler a(toks);
    return a.assemble();
}

TEST(AssemblerTest, SimpleLoadAndHalt)
{
    // LD A,0xFF ; HALT
    std::string src = "LD A,0xFF\nHALT";
    obj out = assemble_source(src);

    // 0x3E = LD A,N ; 0xFF ; 0x76 = HALT
    std::vector<uint8_t> expected = {0x3E, 0xFF, 0x76};
    EXPECT_EQ(out.code, expected);
    EXPECT_TRUE(out.labels.empty());
}

TEST(AssemblerTest, RegisterToRegister)
{
    // LD A,0x01 ; LD B,A
    std::string src = "LD A,0x01\nLD B,A";
    obj out = assemble_source(src);

    // LD A,1 -> 3E 01 ; LD B,A -> 47
    std::vector<uint8_t> expected = {0x3E, 0x01, 0x47};
    EXPECT_EQ(out.code, expected);
}

TEST(AssemblerTest, ByteDirective)
{
    // DB directive should emit raw bytes
    std::string src = "DB 0xAA,0xBB,0x10";
    obj out = assemble_source(src);

    std::vector<uint8_t> expected = {0xAA, 0xBB, 0x10};
    EXPECT_EQ(out.code, expected);
}

TEST(AssemblerTest, IndexedLoad)
{
    // LD A,(IX+5)
    std::string src = "LD A,(IX+5)";
    obj out = assemble_source(src);

    // opcode: DD 7E disp
    ASSERT_EQ(out.code.size(), 3u);
    EXPECT_EQ(out.code[0], 0xDD);
    EXPECT_EQ(out.code[1], 0x7E);
    EXPECT_EQ(out.code[2], 0x05);
}

TEST(AssemblerTest, LabelRecording)
{
    // labels should map to correct offsets
    std::string src = R"(
        START:  LD A,0x10
                NOP
        NEXT:   LD B,0x20
                HALT
    )";
    obj out = assemble_source(src);

    // LD A,N -> 2 bytes, NOP ->1, LD B,N->2, HALT->1
    EXPECT_EQ(out.code.size(), 6u);

    ASSERT_TRUE(out.labels.count("START"));
    EXPECT_EQ(out.labels.at("START"), 0u);

    ASSERT_TRUE(out.labels.count("NEXT"));
    EXPECT_EQ(out.labels.at("NEXT"), 3u);
}

TEST(AssemblerTest, UnknownInstructionThrows)
{
    // Fake mnemonic should cause an exception
    std::string src = "FOO A,0x01";
    EXPECT_THROW(assemble_source(src), std::runtime_error);
}
