// test_int16_edge.cpp — 16-bit integer edge-case and path-coverage tests.
//
// Goals:
//   1. Exercise BOTH code paths in __divuint (atmost7bits / morethan7bits).
//      The branch is taken when (e & 0x80) || d != 0.
//      divisor <= 0x7F  → atmost7bits (9-state loop, 16 iters)
//      divisor >= 0x80  → morethan7bits (8-state loop, 9 iters)
//   2. Exercise the swap/no-swap decision in __mulint.
//   3. Cover __modsint (distinct code path from __smod16: calls
//      __divsint + __get_remainder + ex de,hl).
//   4. Boundary values: max, min, ±1, powers of two.
//   5. Confirm all mod functions now return result in DE.
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"

// ---------------------------------------------------------------------------
// __divuint — atmost7bits path (divisor < 0x80 and D=0)
// ---------------------------------------------------------------------------

TEST(divuint_path_atmost7_divisor_1)
{
    // Divisor=1: atmost7bits, every division step subtracts cleanly
    REQUIRE(g_rt->call16(rt_sym::div16, 500, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)500);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_path_atmost7_divisor_7f)
{
    // Divisor=0x7F (127): last value that stays in atmost7bits
    // 254 / 127 = 2, rem 0
    REQUIRE(g_rt->call16(rt_sym::div16, 254, 0x7F));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)2);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_path_atmost7_remainder)
{
    // 200 / 3 = 66, rem 2  (atmost7bits)
    REQUIRE(g_rt->call16(rt_sym::div16, 200, 3));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)66);
    REQUIRE_EQ(s.hl, (uint16_t)2);
}

TEST(divuint_path_atmost7_quotient_zero)
{
    // 5 / 100 = 0, rem 5  (atmost7bits, quotient=0 from loop)
    REQUIRE(g_rt->call16(rt_sym::div16, 5, 100));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
    REQUIRE_EQ(s.hl, (uint16_t)5);
}

// ---------------------------------------------------------------------------
// __divuint — morethan7bits path (divisor >= 0x80)
// ---------------------------------------------------------------------------

TEST(divuint_path_morethan7_boundary_80)
{
    // Divisor=0x80 (128): first value hitting morethan7bits
    // 256 / 128 = 2, rem 0
    REQUIRE(g_rt->call16(rt_sym::div16, 256, 0x80));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)2);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_path_morethan7_ff)
{
    // Divisor=0xFF (255): morethan7bits (bit7 of E is set)
    // 510 / 255 = 2, rem 0
    REQUIRE(g_rt->call16(rt_sym::div16, 510, 0xFF));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)2);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_path_morethan7_high_byte)
{
    // Divisor=0x100 (256): D=1, forces morethan7bits
    // 1000 / 256 = 3, rem 232
    REQUIRE(g_rt->call16(rt_sym::div16, 1000, 256));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)3);
    REQUIRE_EQ(s.hl, (uint16_t)232);
}

TEST(divuint_path_morethan7_large_divisor)
{
    // Divisor=0x8000: morethan7bits, only 0 or 1 quotient possible
    // 0xFFFF / 0x8000 = 1, rem 0x7FFF
    REQUIRE(g_rt->call16(rt_sym::div16, 0xFFFF, 0x8000));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
    REQUIRE_EQ(s.hl, (uint16_t)0x7FFF);
}

TEST(divuint_path_morethan7_dividend_equals_divisor)
{
    // Exact: dividend=divisor → quotient=1, remainder=0
    REQUIRE(g_rt->call16(rt_sym::div16, 0x8001, 0x8001));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_path_morethan7_remainder)
{
    // 1000 / 300 = 3, rem 100  (morethan7bits: 300 > 127)
    REQUIRE(g_rt->call16(rt_sym::div16, 1000, 300));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)3);
    REQUIRE_EQ(s.hl, (uint16_t)100);
}

// ---------------------------------------------------------------------------
// __divuint — boundary: max dividend / max divisor
// ---------------------------------------------------------------------------

TEST(divuint_max_dividend_max_divisor)
{
    // 0xFFFF / 0xFFFF = 1, rem 0
    REQUIRE(g_rt->call16(rt_sym::div16, 0xFFFF, 0xFFFF));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
    REQUIRE_EQ(s.hl, (uint16_t)0);
}

TEST(divuint_max_dividend_by_2)
{
    // 0xFFFF / 2 = 32767, rem 1
    REQUIRE(g_rt->call16(rt_sym::div16, 0xFFFF, 2));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)32767);
    REQUIRE_EQ(s.hl, (uint16_t)1);
}

// ---------------------------------------------------------------------------
// __mulint — swap / no-swap path
//
// The algorithm swaps operands when multiplicand > multiplier so that the
// smaller value drives the bit-test loop (fewer iterations on average).
// ---------------------------------------------------------------------------

TEST(mulint_noswap_path)
{
    // multiplicand (HL=3) <= multiplier (DE=9): no swap needed
    REQUIRE(g_rt->call16(rt_sym::mul16, 3, 9));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)27);
}

TEST(mulint_swap_path)
{
    // multiplicand (HL=9) > multiplier (DE=3): triggers swap
    REQUIRE(g_rt->call16(rt_sym::mul16, 9, 3));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)27);
}

TEST(mulint_commutativity)
{
    // Both orderings must give the same result
    REQUIRE(g_rt->call16(rt_sym::mul16, 1000, 33));
    auto s1 = g_rt->snap();
    REQUIRE(g_rt->call16(rt_sym::mul16, 33, 1000));
    auto s2 = g_rt->snap();
    REQUIRE_EQ(s1.de, s2.de);
}

TEST(mulint_0xffff_squared_wraps)
{
    // 0xFFFF * 0xFFFF = 0xFFFE0001 → low 16 bits = 0x0001
    REQUIRE(g_rt->call16(rt_sym::mul16, 0xFFFF, 0xFFFF));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)1);
}

TEST(mulint_power_of_2)
{
    // 128 * 256 = 32768 = 0x8000
    REQUIRE(g_rt->call16(rt_sym::mul16, 128, 256));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0x8000);
}

TEST(mulint_1_times_max)
{
    // 1 × 0xFFFF = 0xFFFF (tests loop with one operand = 1 bit set)
    REQUIRE(g_rt->call16(rt_sym::mul16, 1, 0xFFFF));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0xFFFF);
}

TEST(mulint_all_bits_set_x_2)
{
    // 0xFFFF × 2 = 0x1FFFE → low 16 = 0xFFFE
    REQUIRE(g_rt->call16(rt_sym::mul16, 0xFFFF, 2));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0xFFFE);
}

// ---------------------------------------------------------------------------
// __divsint — boundary values
// ---------------------------------------------------------------------------

TEST(divsint_max_pos_by_max_pos)
{
    // 32767 / 32767 = 1, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, 32767, 32767));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)1);
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_pos_by_minus1)
{
    // 100 / -1 = -100
    REQUIRE(g_rt->call16(rt_sym::sdiv16, 100, (uint16_t)(-1)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-100));
}

TEST(divsint_minus1_by_1)
{
    // -1 / 1 = -1, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-1), 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-1));
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

TEST(divsint_large_neg_dividend)
{
    // -30000 / 1000 = -30, rem 0
    REQUIRE(g_rt->call16(rt_sym::sdiv16, (uint16_t)(-30000), 1000));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-30));
    REQUIRE_EQ((int16_t)s.hl, (int16_t)0);
}

// ---------------------------------------------------------------------------
// __moduint — DE = remainder.  More boundary coverage.
// ---------------------------------------------------------------------------

TEST(moduint_divisor_is_power_of_two)
{
    // 0x1234 % 0x100 = 0x34
    REQUIRE(g_rt->call16(rt_sym::mod16, 0x1234, 0x100));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0x34);
}

TEST(moduint_dividend_equals_divisor)
{
    // x % x = 0
    REQUIRE(g_rt->call16(rt_sym::mod16, 300, 300));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)0);
}

TEST(moduint_dividend_one_less_than_divisor)
{
    // (n-1) % n = n-1
    REQUIRE(g_rt->call16(rt_sym::mod16, 299, 300));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)299);
}

TEST(moduint_large_quotient)
{
    // 65000 % 13 = 65000 - (5000*13) = 65000 - 65000 = 0
    REQUIRE(g_rt->call16(rt_sym::mod16, 65000, 13));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de, (uint16_t)(65000 % 13));
}

// ---------------------------------------------------------------------------
// __smod16 — more boundary values (result in DE)
// ---------------------------------------------------------------------------

TEST(smod16_divisor_1_always_zero)
{
    // x % 1 = 0 for all x
    REQUIRE(g_rt->call16(rt_sym::smod16, 12345, 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)0);
}

TEST(smod16_negative_divisor_1)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)(-12345), 1));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)0);
}

TEST(smod16_min_int_mod_positive)
{
    // -32768 % 7: sign follows dividend (negative)
    REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)(-32768), 7));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)((int16_t)(-32768) % 7));
}

TEST(smod16_max_int_mod_small)
{
    REQUIRE(g_rt->call16(rt_sym::smod16, 32767, 100));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(32767 % 100));
}

TEST(smod16_consecutive_values)
{
    // 10 % 3 = 1, 11 % 3 = 2, 12 % 3 = 0
    for (int n = 10; n <= 12; ++n) {
        REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)n, 3));
        auto s = g_rt->snap();
        REQUIRE_EQ((int16_t)s.de, (int16_t)(n % 3));
    }
}

// ---------------------------------------------------------------------------
// __modsint — distinct code path (→ __divsint → __get_remainder → ex de,hl)
// Result in DE.
// ---------------------------------------------------------------------------

TEST(modsint_pos_pos)
{
    REQUIRE(g_rt->call16(rt_sym::modsint, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)1);
}

TEST(modsint_neg_pos)
{
    // -21 % 5 = -1 (sign of dividend)
    REQUIRE(g_rt->call16(rt_sym::modsint, (uint16_t)(-21), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-1));
}

TEST(modsint_pos_neg)
{
    // 21 % -5 = 1 (sign of dividend is positive)
    REQUIRE(g_rt->call16(rt_sym::modsint, 21, (uint16_t)(-5)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)1);
}

TEST(modsint_neg_neg)
{
    // -21 % -5 = -1
    REQUIRE(g_rt->call16(rt_sym::modsint, (uint16_t)(-21), (uint16_t)(-5)));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)(-1));
}

TEST(modsint_exact)
{
    // 20 % 5 = 0
    REQUIRE(g_rt->call16(rt_sym::modsint, 20, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int16_t)s.de, (int16_t)0);
}

TEST(modsint_agrees_with_smod16)
{
    // __modsint and __smod16 should give identical results for signed inputs
    int16_t cases[][2] = {
        {100, 7}, {-100, 7}, {100, -7}, {-100, -7},
        {1, 1}, {32767, 100}, {-32768, 3}
    };
    for (auto& c : cases) {
        REQUIRE(g_rt->call16(rt_sym::modsint, (uint16_t)c[0], (uint16_t)c[1]));
        int16_t modsint_result = (int16_t)g_rt->snap().de;
        REQUIRE(g_rt->call16(rt_sym::smod16, (uint16_t)c[0], (uint16_t)c[1]));
        int16_t smod16_result  = (int16_t)g_rt->snap().de;
        REQUIRE_EQ(modsint_result, smod16_result);
    }
}

// ---------------------------------------------------------------------------
// __divschar / __modschar — more sign combinations
// ---------------------------------------------------------------------------

TEST(divschar_remainder_positive)
{
    // 21 / 5: quotient in DE low byte, unsigned remainder in HL low byte
    REQUIRE(g_rt->call8(rt_sym::divschar, 21, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int8_t)(s.de & 0xFF), (int8_t)4);
    REQUIRE_EQ((int8_t)(s.hl & 0xFF), (int8_t)1);
}

TEST(modschar_neg_dividend)
{
    // -21 % 5 = -1
    REQUIRE(g_rt->call8(rt_sym::modschar, (uint8_t)(-21), 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int8_t)(s.de & 0xFF), (int8_t)(-1));
}

TEST(modschar_exact)
{
    REQUIRE(g_rt->call8(rt_sym::modschar, 20, 5));
    auto s = g_rt->snap();
    REQUIRE_EQ((int8_t)(s.de & 0xFF), (int8_t)0);
}

TEST(moduschar_larger_values)
{
    // 200 % 9 = 200 - 22*9 = 200 - 198 = 2
    REQUIRE(g_rt->call8(rt_sym::moduschar, 200, 9));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.de & 0xFF, (uint16_t)2);
}
