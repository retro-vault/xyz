// test_shifts.cpp — 16-bit shift runtime function tests.
//
// Covers: __shl16, __shr16s (arithmetic), __shr16u (logical)
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"

// ---------------------------------------------------------------------------
// __shl16 — logical left shift, HL = input, B = count
// ---------------------------------------------------------------------------

TEST(shl16_shift_by_zero)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 0x1234, 0));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x1234);
}

TEST(shl16_shift_by_1)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 1, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)2);
}

TEST(shl16_shift_by_4)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 1, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)16);
}

TEST(shl16_shift_by_8)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 1, 8));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)256);
}

TEST(shl16_shift_to_msb)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 1, 15));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x8000);
}

TEST(shl16_shifts_out_of_range)
{
    // shifting all bits out produces 0
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 0x0001, 16));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(shl16_value_0xFF00)
{
    REQUIRE(g_rt->call_shift(rt_sym::shl16, 0xFF00, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0xFE00);
}

// ---------------------------------------------------------------------------
// __shr16s — arithmetic (signed) right shift
// ---------------------------------------------------------------------------

TEST(shr16s_zero_count)
{
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, 0x1234, 0));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x1234);
}

TEST(shr16s_positive_shift_4)
{
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, 16, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)1);
}

TEST(shr16s_negative_shift_1)
{
    // -16 >> 1 = -8 (arithmetic: sign preserved)
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, (uint16_t)(-16), 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(-8));
}

TEST(shr16s_negative_shift_4)
{
    // -16 >> 4 = -1
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, (uint16_t)(-16), 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(-1));
}

TEST(shr16s_min_int_shift_1)
{
    // -32768 >> 1 = -16384
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, 0x8000, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(-16384));
}

TEST(shr16s_min_int_shift_15)
{
    // -32768 >> 15 = -1
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, 0x8000, 15));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(-1));
}

TEST(shr16s_all_ones_shift)
{
    // -1 >> 8 = -1 (arithmetic)
    REQUIRE(g_rt->call_shift(rt_sym::shr16s, 0xFFFF, 8));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(-1));
}

// ---------------------------------------------------------------------------
// __shr16u — logical (unsigned) right shift
// ---------------------------------------------------------------------------

TEST(shr16u_zero_count)
{
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 0x1234, 0));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x1234);
}

TEST(shr16u_shift_1)
{
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 16, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)8);
}

TEST(shr16u_msb_shift_1)
{
    // 0x8000 >> 1 = 0x4000 (logical: no sign extension)
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 0x8000, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x4000);
}

TEST(shr16u_all_ones_shift_1)
{
    // 0xFFFF >> 1 = 0x7FFF
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 0xFFFF, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0x7FFF);
}

TEST(shr16u_all_ones_shift_15)
{
    // 0xFFFF >> 15 = 1
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 0xFFFF, 15));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)1);
}

TEST(shr16u_all_ones_shift_16)
{
    // logical: shift past word width → 0
    REQUIRE(g_rt->call_shift(rt_sym::shr16u, 0xFFFF, 16));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0);
}
