// test_int32.cpp — 32-bit integer runtime function tests.
//
// Covers: __mullong, __divulong, __divslong, __modulong, __modslong,
//         ___mulsint2slong, ___muluint2ulong
//
// ABI: first arg in DE:HL (DE=low16, HL=high16), second arg on stack.
// Result in DE:HL (DE=low16, HL=high16): value = (HL<<16)|DE.
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"

// ---------------------------------------------------------------------------
// __mullong — signed 32-bit multiply
// ---------------------------------------------------------------------------

TEST(mullong_3x7)
{
    REQUIRE(g_rt->call32(rt_sym::mul32, 3, 7));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)21);
}

TEST(mullong_1000x1000)
{
    REQUIRE(g_rt->call32(rt_sym::mul32, 1000, 1000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)1000000);
}

TEST(mullong_zero_times_n)
{
    REQUIRE(g_rt->call32(rt_sym::mul32, 0, 0x12345678));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(mullong_large_positive)
{
    // 100000 * 32767 = 3,276,700,000 — fits in 32 bits
    REQUIRE(g_rt->call32(rt_sym::mul32, 100000, 32767));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)100000 * 32767);
}

TEST(mullong_neg_x_pos)
{
    // -5 * 6 = -30
    REQUIRE(g_rt->call32(rt_sym::mul32, (uint32_t)(-5), 6));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-30));
}

TEST(mullong_neg_x_neg)
{
    // -7 * -8 = 56
    REQUIRE(g_rt->call32(rt_sym::mul32, (uint32_t)(-7), (uint32_t)(-8)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)56);
}

TEST(mullong_identity)
{
    REQUIRE(g_rt->call32(rt_sym::mul32, 0xDEAD, 1));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0xDEAD);
}

TEST(mullong_overflow_wraps)
{
    // 0x10000 * 0x10000 = 0x100000000 → wraps to 0
    REQUIRE(g_rt->call32(rt_sym::mul32, 0x10000, 0x10000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

// ---------------------------------------------------------------------------
// __divulong — unsigned 32-bit divide
// ---------------------------------------------------------------------------

TEST(divulong_basic)
{
    REQUIRE(g_rt->call32(rt_sym::div32, 100, 5));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)20);
}

TEST(divulong_exact)
{
    REQUIRE(g_rt->call32(rt_sym::div32, 1000000, 1000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)1000);
}

TEST(divulong_large_dividend)
{
    REQUIRE(g_rt->call32(rt_sym::div32, 0xFFFFFFFF, 0xFFFF));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)(0xFFFFFFFF / 0xFFFF));
}

TEST(divulong_divisor_1)
{
    REQUIRE(g_rt->call32(rt_sym::div32, 0x12345678, 1));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x12345678);
}

TEST(divulong_dividend_smaller)
{
    REQUIRE(g_rt->call32(rt_sym::div32, 3, 10));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(divulong_power_of_two)
{
    // 0x80000000 / 0x100 = 0x800000
    REQUIRE(g_rt->call32(rt_sym::div32, 0x80000000, 0x100));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x800000);
}

// ---------------------------------------------------------------------------
// __divslong — signed 32-bit divide
// ---------------------------------------------------------------------------

TEST(divslong_pos_pos)
{
    REQUIRE(g_rt->call32(rt_sym::sdiv32, 100, 4));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)25);
}

TEST(divslong_neg_pos)
{
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)(-100), 4));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-25));
}

TEST(divslong_pos_neg)
{
    REQUIRE(g_rt->call32(rt_sym::sdiv32, 100, (uint32_t)(-4)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-25));
}

TEST(divslong_neg_neg)
{
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)(-100), (uint32_t)(-4)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)25);
}

TEST(divslong_remainder)
{
    // 101 / 10 = 10 (truncated toward 0)
    REQUIRE(g_rt->call32(rt_sym::sdiv32, 101, 10));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)10);
}

TEST(divslong_large)
{
    REQUIRE(g_rt->call32(rt_sym::sdiv32, 1000000000, 1000));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1000000);
}

TEST(divslong_min_divided_by_minus1)
{
    // INT32_MIN / -1 wraps (UB in C, but asm should give some result)
    // Just check it doesn't hang
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)0x80000000, (uint32_t)(-1)));
    // We don't assert specific value; just that it returned
}

// ---------------------------------------------------------------------------
// __modulong — unsigned 32-bit modulus
// ---------------------------------------------------------------------------

TEST(modulong_basic)
{
    REQUIRE(g_rt->call32(rt_sym::mod32, 101, 10));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)1);
}

TEST(modulong_exact)
{
    REQUIRE(g_rt->call32(rt_sym::mod32, 100, 10));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(modulong_large)
{
    REQUIRE(g_rt->call32(rt_sym::mod32, 0xFFFFFFFF, 1000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)(0xFFFFFFFF % 1000));
}

TEST(modulong_zero_dividend)
{
    REQUIRE(g_rt->call32(rt_sym::mod32, 0, 7));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(modulong_divisor_is_power_of_two)
{
    // 0x12345678 % 0x100 = 0x78
    REQUIRE(g_rt->call32(rt_sym::mod32, 0x12345678, 0x100));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x78);
}

// ---------------------------------------------------------------------------
// __modslong — signed 32-bit modulus
// ---------------------------------------------------------------------------

TEST(modslong_pos_pos)
{
    REQUIRE(g_rt->call32(rt_sym::smod32, 101, 10));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1);
}

TEST(modslong_neg_pos)
{
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)(-101), 10));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

TEST(modslong_pos_neg)
{
    REQUIRE(g_rt->call32(rt_sym::smod32, 101, (uint32_t)(-10)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1);
}

TEST(modslong_neg_neg)
{
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)(-101), (uint32_t)(-10)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

TEST(modslong_exact)
{
    REQUIRE(g_rt->call32(rt_sym::smod32, 100, 10));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0);
}

// ---------------------------------------------------------------------------
// ___mulsint2slong — signed 16×16→32
// ---------------------------------------------------------------------------

TEST(mulsint2slong_3x7)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, 3, 7));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)21);
}

TEST(mulsint2slong_neg_x_pos)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, (uint16_t)(-100), 300));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-30000));
}

TEST(mulsint2slong_max_positive)
{
    // 32767 * 32767 = 1,073,676,289
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, 32767, 32767));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)32767 * 32767);
}

TEST(mulsint2slong_min_x_max)
{
    // -32768 * 32767 = -1,073,709,056
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, (uint16_t)(-32768), 32767));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-32768) * 32767);
}

// ---------------------------------------------------------------------------
// ___muluint2ulong — unsigned 16×16→32
// ---------------------------------------------------------------------------

TEST(muluint2ulong_basic)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 300, 400));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)120000);
}

TEST(muluint2ulong_max)
{
    // 65535 * 65535 = 4,294,836,225
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 65535, 65535));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)65535 * (uint32_t)65535);
}

TEST(muluint2ulong_zero)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 0, 65535));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}
