// test_int16.cpp — 16-bit integer runtime function tests.
//
// Covers: __mulint/__mul16, __divuint/__div16, __divsint/__sdiv16,
//         __moduint, __modsint, __smod16,
//         __mulschar, __divschar, __divuschar, __modschar, __moduschar,
//         __mulsuchar, __muluschar, __divsuchar, __modsuchar
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"

// ---------------------------------------------------------------------------
// 16-bit multiply
// ---------------------------------------------------------------------------

TEST(mulint_3x7)
{
    REQUIRE(g_rt->call16(rt_sym::mul16, 3, 7));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)21);
}

TEST(mulint_0xa_x_0xb)
{
    REQUIRE(g_rt->call16(rt_sym::mul16, 100, 200));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)20000);
}

TEST(mulint_zero_times_n)
{
    REQUIRE(g_rt->call16(rt_sym::mul16, 0, 12345));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

TEST(mulint_n_times_zero)
{
    REQUIRE(g_rt->call16(rt_sym::mul16, 12345, 0));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

TEST(mulint_max_bytes)
{
    // 255 * 255 = 65025
    REQUIRE(g_rt->call16(rt_sym::mul16, 255, 255));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)65025);
}

TEST(mulint_wraps_16bit)
{
    // 256 * 256 = 65536 → truncated to 0 in 16-bit
    REQUIRE(g_rt->call16(rt_sym::mul16, 256, 256));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

TEST(mulint_signed_neg_result)
{
    // (uint16_t)(-3) * 5 → low 16 bits of -15 = 0xFFF1
    REQUIRE(g_rt->call16(rt_sym::mul16, (uint16_t)(-3), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)(-15));
}

TEST(mulint_one_x_n)
{
    REQUIRE(g_rt->call16(rt_sym::mul16, 1, 1000));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1000);
}

// ---------------------------------------------------------------------------
// 16-bit unsigned divide
// ---------------------------------------------------------------------------

TEST(divuint_20_by_4)
{
    REQUIRE(g_rt->call16(rt_sym::div16, 20, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)5);   // quotient
    REQUIRE_EQ(s.hl, (uint16_t)0);   // remainder
}

TEST(divuint_21_by_5)
{
    REQUIRE(g_rt->call16(rt_sym::div16, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)4);
    REQUIRE_EQ(s.hl, (uint16_t)1);
}

TEST(divuint_65535_by_255)
{
    REQUIRE(g_rt->call16(rt_sym::div16, 65535, 255));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)257);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_zero_dividend)
{
    REQUIRE(g_rt->call16(rt_sym::div16, 0, 7));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_divisor_larger)
{
    // 3 / 10 = 0 rem 3
    REQUIRE(g_rt->call16(rt_sym::div16, 3, 10));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
    REQUIRE_EQ(s.hl, (uint16_t)3);
}

TEST(divuint_large_dividend)
{
    // 60000 / 1000 = 60 rem 0
    REQUIRE(g_rt->call16(rt_sym::div16, 60000, 1000));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)60);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_divisor_is_1)
{
    REQUIRE(g_rt->call16(rt_sym::div16, 1234, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1234);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

// ---------------------------------------------------------------------------
// 16-bit signed divide
// ---------------------------------------------------------------------------

TEST(divsint_pos_pos)
{
    // 20 / 4 = 5, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, 20, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)5);
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_neg_pos)
{
    // -20 / 4 = -5, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-20), 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-5));
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_pos_neg)
{
    // 20 / -4 = -5, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, 20, (uint16_t)(-4)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-5));
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_neg_neg)
{
    // -20 / -4 = 5, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-20), (uint16_t)(-4)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)5);
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_remainder_pos)
{
    // 21 / 5 = 4, rem 1
    REQUIRE(g_rt->call16(rt_sym::sdiv16, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)4);
    REQUIRE_EQ((int16_t)s.hl, (int16_t)1);
}

TEST(divsint_remainder_neg_dividend)
{
    // __divsint sign-corrects DE (quotient) but leaves HL as the UNSIGNED
    // remainder from __divuint. Call __get_remainder for signed remainder.
    // -21 / 5 = -4 quotient, unsigned_remainder = 1 (= 21 - 4*5)
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-21), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-4));
    REQUIRE_EQ((int16_t)s.hl, (int16_t)(1));
}

TEST(divsint_divide_by_1)
{
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-999), 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-999));
}

// ---------------------------------------------------------------------------
// 16-bit unsigned modulus
// ---------------------------------------------------------------------------

TEST(moduint_21_mod_5)
{
    REQUIRE(g_rt->call16(rt_sym::mod16, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
}

TEST(moduint_20_mod_4)
{
    REQUIRE(g_rt->call16(rt_sym::mod16, 20, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

TEST(moduint_7_mod_3)
{
    REQUIRE(g_rt->call16(rt_sym::mod16, 7, 3));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
}

TEST(moduint_large_values)
{
    // 65535 % 256 = 255
    REQUIRE(g_rt->call16(rt_sym::mod16, 65535, 256));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)255);
}

TEST(moduint_zero_dividend)
{
    REQUIRE(g_rt->call16(rt_sym::mod16, 0, 7));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

// ---------------------------------------------------------------------------
// 16-bit signed modulus (__smod16 — direct implementation)
// ---------------------------------------------------------------------------

TEST(smod16_pos_pos)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)1);
}

TEST(smod16_neg_pos)
{
    // C semantics: sign of remainder = sign of dividend
    REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)(-21), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-1));
}

TEST(smod16_pos_neg)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, 21, (uint16_t)(-5)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)1);
}

TEST(smod16_neg_neg)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)(-21), (uint16_t)(-5)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-1));
}

TEST(smod16_exact_division)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, 20, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)0);
}

TEST(smod16_dividend_smaller)
{
    // 3 % 10 = 3
    REQUIRE(g_rt->call16(rt_sym::smod16, 3, 10));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)3);
}

// ---------------------------------------------------------------------------
// 8-bit signed multiply (__mulschar): A=lhs, L=rhs → DE = 16-bit product
// ---------------------------------------------------------------------------

TEST(mulschar_3x7)
{
    REQUIRE(g_rt->call8(rt_sym::mulschar, 3, 7));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)21);
}

TEST(mulschar_neg_x_pos)
{
    REQUIRE(g_rt->call8(rt_sym::mulschar, (uint8_t)(-3), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-15));
}

TEST(mulschar_neg_x_neg)
{
    REQUIRE(g_rt->call8(rt_sym::mulschar, (uint8_t)(-4), (uint8_t)(-3)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)12);
}

TEST(mulschar_max_values)
{
    // 127 * 127 = 16129
    REQUIRE(g_rt->call8(rt_sym::mulschar, 127, 127));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)16129);
}

// ---------------------------------------------------------------------------
// 8-bit signed divide (__divschar): A=dividend, L=divisor → DE=quot, HL=rem
// ---------------------------------------------------------------------------

TEST(divschar_pos_div)
{
    REQUIRE(g_rt->call8(rt_sym::divschar, 20, 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int8_t)(s.de & 0xFF), (int8_t)5);
}

TEST(divschar_neg_dividend)
{
    REQUIRE(g_rt->call8(rt_sym::divschar, (uint8_t)(-20), 4));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-5));
}

// ---------------------------------------------------------------------------
// 8-bit unsigned divide (__divuschar): A=dividend, L=divisor
// ---------------------------------------------------------------------------

TEST(divuschar_basic)
{
    REQUIRE(g_rt->call8(rt_sym::divuschar, 100, 10));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de & 0xFF, (uint16_t)10);
}

// ---------------------------------------------------------------------------
// 8-bit signed modulus (__modschar)
// ---------------------------------------------------------------------------

TEST(modschar_basic)
{
    REQUIRE(g_rt->call8(rt_sym::modschar, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int8_t)(s.de & 0xFF), (int8_t)1);
}

// ---------------------------------------------------------------------------
// 8-bit unsigned modulus (__moduschar)
// ---------------------------------------------------------------------------

TEST(moduschar_basic)
{
    REQUIRE(g_rt->call8(rt_sym::moduschar, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de & 0xFF, (uint16_t)1);
}
