//
// test_formatter.cpp — unit tests for shared Z80 syntax formatting.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <string>
#include <vector>

#include <xz80/formatter.h>

TEST(formatter_sdcc_indexed_immediate)
{
    const auto line = xz80::format_instruction(
        xz80::syntax_style::sdcc, "LD", {"A", "#0x42", "(IX+5)"});
    REQUIRE_EQ(xz80::render_line(line), std::string("ld\ta, #0x42, 5(ix)"));
}

TEST(formatter_gnu_indexed_immediate)
{
    const auto line = xz80::format_instruction(
        xz80::syntax_style::gnu, "LD", {"A", "#0x42", "(IX+label)"});
    REQUIRE_EQ(xz80::render_line(line), std::string("ld\ta, 0x42, (ix+label)"));
}
