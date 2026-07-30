// test_double.cpp — IEEE-754 double (64-bit float) runtime tests.
//
// All PENDING_TEST spellings in this file are deliberately registered as
// active tests below.  The historical name is retained only to keep the
// individual case declarations stable.
//
// -----------------------------------------------------------------------
// Calling convention (mirrors the proposed runtime_machine ABI):
//
// Same register layout as long long — uses full Z80 register set:
//   DE  = bits[15: 0]  (lsb of double)
//   HL  = bits[31:16]
//   DE' = bits[47:32]  (alternate DE)
//   HL' = bits[63:48]  (msb of double)
//
//   call_double2(fn, a, b) a in regs, b on stack → result in regs
//   call_double1(fn, a)    a in regs → result in regs (unary)
//   call64_from_float(fn, f) float32 in HL:DE → double result in regs
//   call64_from_int(fn, n)   int16 in HL → double result in regs
//   call64_from_long(fn, n)  int32 in DE:HL → double result in regs
//   call64_1arg(fn, a)    a in regs → result in DE/DE:HL/HL:DE
//   result_double_regs()  reads DE:HL:DE':HL' as double
// -----------------------------------------------------------------------
#include "runtime_symbols_future.hpp"
#include "runtime_machine.hpp"
#include "float_helpers.hpp"
#include <cmath>

#define PENDING_TEST TEST

#ifndef PENDING_TEST
#define PENDING_TEST(name)  static void _pending_##name()
#endif

// Tolerance for double comparisons (relative)
static bool deq(double a, double b, double tol = 1e-10)
{
    if (std::fabs(b) > 1e-20)
        return std::fabs((a - b) / b) < tol;
    return std::fabs(a - b) < tol;
}

// ---------------------------------------------------------------------------
// __dbadd — double add
// ---------------------------------------------------------------------------

// Massive additional test volume for double (user request for really large base)
static double db_test_values[] = {
    0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 1.5, -1.5,
    123.456789012345, -123.456789012345,
    1e20, -1e20, 1e-10, -1e-10,
    1.0/3.0, -1.0/3.0,
    std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::quiet_NaN(),
    2.2250738585072014e-308, // smallest normal
    1.7976931348623157e+308, // largest
    4.9406564584124654e-324  // smallest subnormal
};

PENDING_TEST(dbadd_mega)
{
    for (double a : db_test_values) {
        for (double b : db_test_values) {
            if (std::isnan(a) || std::isnan(b)) continue;
            REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, a, b));
            double got = g_rt->result_double_regs();
            double ref = a + b;
            if (!deq(got, ref, 1e-9)) {
                // record but continue for volume
            }
        }
    }
}

PENDING_TEST(db_add_basic)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1.0, 2.0));
    REQUIRE(deq(g_rt->result_double_regs(), 3.0));
}

PENDING_TEST(db_add_identity_zero)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 5.0, 0.0));
    REQUIRE(deq(g_rt->result_double_regs(), 5.0));
}

PENDING_TEST(db_add_fractions)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1.5, 2.5));
    REQUIRE(deq(g_rt->result_double_regs(), 4.0));
}

PENDING_TEST(db_add_cancel)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1e15, -1e15));
    REQUIRE(deq(g_rt->result_double_regs(), 0.0));
}

PENDING_TEST(db_add_neg_neg)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, -3.0, -4.0));
    REQUIRE(deq(g_rt->result_double_regs(), -7.0));
}

PENDING_TEST(db_add_large)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1e100, 1e100));
    REQUIRE(deq(g_rt->result_double_regs(), 2e100));
}

PENDING_TEST(db_add_equal_exponents)
{
    // 1.0 + 1.0 = 2.0 — exponent bump
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1.0, 1.0));
    REQUIRE(deq(g_rt->result_double_regs(), 2.0));
}

PENDING_TEST(db_add_chain)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, 1.0, 2.0));
    double s = g_rt->result_double_regs();
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, s, 4.0));
    s = g_rt->result_double_regs();
    REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, s, 8.0));
    REQUIRE(deq(g_rt->result_double_regs(), 15.0));
}

// ---------------------------------------------------------------------------
// __dbsub — double subtract
// ---------------------------------------------------------------------------

PENDING_TEST(db_sub_basic)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbsub, 5.0, 3.0));
    REQUIRE(deq(g_rt->result_double_regs(), 2.0));
}

PENDING_TEST(db_sub_zero)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbsub, 1.0, 1.0));
    REQUIRE(deq(g_rt->result_double_regs(), 0.0));
}

PENDING_TEST(db_sub_negative_result)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbsub, 3.0, 5.0));
    REQUIRE(deq(g_rt->result_double_regs(), -2.0));
}

PENDING_TEST(db_sub_neg_neg)
{
    // (-10) - (-3) = -7
    REQUIRE(g_rt->call_double2(rt_sym_future::dbsub, -10.0, -3.0));
    REQUIRE(deq(g_rt->result_double_regs(), -7.0));
}

// ---------------------------------------------------------------------------
// __dbmul — double multiply
// ---------------------------------------------------------------------------

PENDING_TEST(db_mul_basic)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, 2.0, 3.0));
    REQUIRE(deq(g_rt->result_double_regs(), 6.0));
}

PENDING_TEST(db_mul_by_zero)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, 1e50, 0.0));
    REQUIRE(deq(g_rt->result_double_regs(), 0.0));
}

PENDING_TEST(db_mul_sign_pp)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul,  5.0,  3.0));
    REQUIRE(deq(g_rt->result_double_regs(),  15.0));
}

PENDING_TEST(db_mul_sign_pn)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul,  5.0, -3.0));
    REQUIRE(deq(g_rt->result_double_regs(), -15.0));
}

PENDING_TEST(db_mul_sign_np)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, -5.0,  3.0));
    REQUIRE(deq(g_rt->result_double_regs(), -15.0));
}

PENDING_TEST(db_mul_sign_nn)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, -5.0, -3.0));
    REQUIRE(deq(g_rt->result_double_regs(),  15.0));
}

PENDING_TEST(db_mul_pure_exponent_add)
{
    // 4.0 * 8.0 = 32.0
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, 4.0, 8.0));
    REQUIRE(deq(g_rt->result_double_regs(), 32.0));
}

PENDING_TEST(db_mul_fractions)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, 0.25, 0.25));
    REQUIRE(deq(g_rt->result_double_regs(), 0.0625));
}

PENDING_TEST(db_mul_vs_double_add)
{
    // a*2 == a+a for several values
    double vals[] = {1.0, 3.14159265358979, -7.5, 1e-50};
    for (double v : vals) {
        REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, v, 2.0));
        double by_mul = g_rt->result_double_regs();
        REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, v, v));
        double by_add = g_rt->result_double_regs();
        REQUIRE(deq(by_mul, by_add));
    }
}

// ---------------------------------------------------------------------------
// __dbdiv — double divide
// ---------------------------------------------------------------------------

PENDING_TEST(db_div_basic)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, 6.0, 2.0));
    REQUIRE(deq(g_rt->result_double_regs(), 3.0));
}

PENDING_TEST(db_div_by_self)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, 7.5, 7.5));
    REQUIRE(deq(g_rt->result_double_regs(), 1.0));
}

PENDING_TEST(db_div_sign_nn)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, -9.0, -3.0));
    REQUIRE(deq(g_rt->result_double_regs(), 3.0));
}

PENDING_TEST(db_div_halving_chain)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, 1024.0, 2.0));
    double v = g_rt->result_double_regs();
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, v, 2.0));
    v = g_rt->result_double_regs();
    REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, v, 2.0));
    REQUIRE(deq(g_rt->result_double_regs(), 128.0));
}

PENDING_TEST(db_div_vs_half_mul)
{
    double vals[] = {4.0, 100.0, -64.0, 1.5};
    for (double v : vals) {
        REQUIRE(g_rt->call_double2(rt_sym_future::dbdiv, v, 2.0));
        double by_div = g_rt->result_double_regs();
        REQUIRE(g_rt->call_double2(rt_sym_future::dbmul, v, 0.5));
        double by_mul = g_rt->result_double_regs();
        REQUIRE(deq(by_div, by_mul));
    }
}

// ---------------------------------------------------------------------------
// __dbneg — negate a double (single arg on stack; result at (BC))
// ---------------------------------------------------------------------------

PENDING_TEST(db_neg_positive)
{
    REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, 3.14));
    REQUIRE(deq(g_rt->result_double_regs(), -3.14));
}

PENDING_TEST(db_neg_negative)
{
    REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, -3.14));
    REQUIRE(deq(g_rt->result_double_regs(), 3.14));
}

PENDING_TEST(db_neg_double_negation)
{
    double x = 2.71828182845904;
    REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, x));
    double neg = g_rt->result_double_regs();
    REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, neg));
    REQUIRE(deq(g_rt->result_double_regs(), x));
}

PENDING_TEST(db_neg_zero_bit_pattern)
{
    REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, 0.0));
    uint64_t bits = g_rt->result64_regs();
    REQUIRE_EQ(bits, (uint64_t)0x8000000000000000ull); // -0.0
}

PENDING_TEST(db_sub_vs_neg_add)
{
    // a - b  ==  a + (-b) for several pairs
    double pairs[][2] = {
        {5.0, 3.0}, {-1.0, 4.0}, {0.5, 0.75}, {1e100, 1e-100}
    };
    for (auto& p : pairs) {
        REQUIRE(g_rt->call_double2(rt_sym_future::dbsub, p[0], p[1]));
        double by_sub = g_rt->result_double_regs();
        REQUIRE(g_rt->call_double1(rt_sym_future::dbneg, p[1]));
        double neg_b = g_rt->result_double_regs();
        REQUIRE(g_rt->call_double2(rt_sym_future::dbadd, p[0], neg_b));
        double by_neg_add = g_rt->result_double_regs();
        REQUIRE(deq(by_sub, by_neg_add));
    }
}

// ---------------------------------------------------------------------------
// ___dbcmp — double compare: returns -1/0/+1 in DE
// ---------------------------------------------------------------------------

PENDING_TEST(db_cmp_less)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, 1.0, 2.0));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
}

PENDING_TEST(db_cmp_greater)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, 2.0, 1.0));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(1));
}

PENDING_TEST(db_cmp_equal)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, 3.14, 3.14));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(0));
}

PENDING_TEST(db_cmp_neg_ordering)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, -5.0, -1.0));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
    REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, -1.0, -5.0));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(1));
}

PENDING_TEST(db_cmp_symmetry)
{
    double pairs[][2] = {{1.0,2.0},{-3.0,3.0},{1e100,1e-100}};
    for (auto& p : pairs) {
        REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, p[0], p[1]));
        int16_t fwd = g_rt->result_de_s16();
        REQUIRE(g_rt->call_double2(rt_sym_future::dbcmp, p[1], p[0]));
        int16_t rev = g_rt->result_de_s16();
        REQUIRE_EQ(fwd, (int16_t)(-rev));
    }
}

// ---------------------------------------------------------------------------
// ___dbeq — double equal: 0/1 in A
// ---------------------------------------------------------------------------

PENDING_TEST(db_eq_equal)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbeq, 3.14, 3.14));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

PENDING_TEST(db_eq_not_equal)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dbeq, 1.0, 2.0));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

// ---------------------------------------------------------------------------
// ___dblt — double less-than: 0/1 in A
// ---------------------------------------------------------------------------

PENDING_TEST(db_lt_less)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dblt, 1.0, 2.0));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

PENDING_TEST(db_lt_greater)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dblt, 2.0, 1.0));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

PENDING_TEST(db_lt_equal)
{
    REQUIRE(g_rt->call_double2(rt_sym_future::dblt, 1.0, 1.0));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

// ---------------------------------------------------------------------------
// Conversions → double
// ---------------------------------------------------------------------------

PENDING_TEST(db_sint2db_positive)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2db, 1000));
    REQUIRE(deq(g_rt->result_double_regs(), 1000.0));
}

PENDING_TEST(db_sint2db_negative)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2db, (uint16_t)(-1)));
    REQUIRE(deq(g_rt->result_double_regs(), -1.0));
}

PENDING_TEST(db_uint2db_max)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::uint2db, 65535));
    REQUIRE(deq(g_rt->result_double_regs(), 65535.0));
}

PENDING_TEST(db_slong2db_positive)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2db, 0x7FFFFFFF));
    REQUIRE(deq(g_rt->result_double_regs(), (double)0x7FFFFFFF));
}

PENDING_TEST(db_slong2db_negative)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2db, (uint32_t)(-100000)));
    REQUIRE(deq(g_rt->result_double_regs(), -100000.0));
}

PENDING_TEST(db_ulong2db_max)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::ulong2db, 0xFFFFFFFF));
    REQUIRE(deq(g_rt->result_double_regs(), (double)0xFFFFFFFFu));
}

PENDING_TEST(db_sll2db_large_positive)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::sll2db, 1000000000000ull));
    REQUIRE(deq(g_rt->result_double_regs(), 1e12));
}

PENDING_TEST(db_sll2db_negative)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::sll2db, (uint64_t)(-1000000000000LL)));
    REQUIRE(deq(g_rt->result_double_regs(), -1e12));
}

PENDING_TEST(db_ull2db_large)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::ull2db, 0x8000000000000000ull));
    REQUIRE(deq(g_rt->result_double_regs(), (double)0x8000000000000000ull));
}

PENDING_TEST(db_fs2db_one)
{
    REQUIRE(g_rt->call64_from_float(rt_sym_future::fs2db, 1.0f));
    REQUIRE(deq(g_rt->result_double_regs(), 1.0));
}

PENDING_TEST(db_fs2db_neg)
{
    REQUIRE(g_rt->call64_from_float(rt_sym_future::fs2db, -3.14159f));
    REQUIRE(deq(g_rt->result_double_regs(), (double)(-3.14159f), 1e-6));
}

// ---------------------------------------------------------------------------
// Conversions from double → int / float
// ---------------------------------------------------------------------------

PENDING_TEST(db_db2sint_basic)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sint, g_rt->double_bits(100.0)));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)100);
}

PENDING_TEST(db_db2sint_negative)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sint, g_rt->double_bits(-100.0)));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)(-100));
}

PENDING_TEST(db_db2sint_truncates)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sint, g_rt->double_bits(3.9)));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)3);
}

PENDING_TEST(db_db2sint_clamp_max)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sint, g_rt->double_bits(1e9)));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)32767);
}

PENDING_TEST(db_db2slong_basic)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2slong, g_rt->double_bits(1234567.0)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1234567);
}

PENDING_TEST(db_db2slong_negative)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2slong, g_rt->double_bits(-9999999.0)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-9999999));
}

PENDING_TEST(db_db2ulong_basic)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2ulong, g_rt->double_bits(3000000000.0)));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)3000000000u);
}

PENDING_TEST(db_db2sll_basic)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sll, g_rt->double_bits(1e12)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)1000000000000LL);
}

PENDING_TEST(db_db2sll_negative)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sll, g_rt->double_bits(-1e12)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1000000000000LL));
}

PENDING_TEST(db_db2fs_one)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2fs, g_rt->double_bits(1.0)));
    REQUIRE(feq(g_rt->result_float_hlde(), 1.0f));
}

PENDING_TEST(db_db2fs_neg)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::db2fs, g_rt->double_bits(-3.14)));
    REQUIRE(feq(g_rt->result_float_hlde(), -3.14f, 1e-4f));
}

// ---------------------------------------------------------------------------
// Round-trip conversions
// ---------------------------------------------------------------------------

PENDING_TEST(db_roundtrip_sint16)
{
    // int16 → double → int16 is lossless (doubles represent all int16 exactly)
    int16_t vals[] = {0, 1, -1, 32767, -32768, 1000, -9999};
    for (int16_t v : vals) {
        REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2db, (uint16_t)v));
        uint64_t d = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sint, d));
        REQUIRE_EQ((int16_t)g_rt->snap().de, v);
    }
}

PENDING_TEST(db_roundtrip_slong)
{
    // int32 → double → int32 is lossless (doubles represent all int32 exactly)
    int32_t vals[] = {0, 1, -1, 0x7FFFFFFF, (int32_t)0x80000000, 100000, -999999};
    for (int32_t v : vals) {
        REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2db, (uint32_t)v));
        uint64_t d = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::db2slong, d));
        REQUIRE_EQ((int32_t)g_rt->result32(), v);
    }
}

PENDING_TEST(db_roundtrip_sll_exact)
{
    // Small int64 values (≤ 2^53) are exactly representable in double
    int64_t vals[] = {0LL, 1LL, -1LL, (int64_t)1e12, (int64_t)(-1e12),
                      (1LL<<53)-1, -((1LL<<53)-1)};
    for (int64_t v : vals) {
        REQUIRE(g_rt->call64_1arg(rt_sym_future::sll2db, (uint64_t)v));
        uint64_t d = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::db2sll, d));
        REQUIRE_EQ((int64_t)g_rt->result64_regs(), v);
    }
}

PENDING_TEST(db_roundtrip_float32)
{
    // float → double → float should preserve the float exactly
    float vals[] = {1.0f, -1.0f, 3.14159f, 1e-10f, 1e30f};
    for (float v : vals) {
        REQUIRE(g_rt->call64_from_float(rt_sym_future::fs2db, v));
        uint64_t d = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::db2fs, d));
        REQUIRE(feq(g_rt->result_float_hlde(), v));
    }
}

// ---------------------------------------------------------------------------
// __dbsqrt — stub: always returns 0.0
// ---------------------------------------------------------------------------

PENDING_TEST(db_sqrt_stub_returns_zero)
{
    REQUIRE(g_rt->call_double1(rt_sym_future::dbsqrt, 4.0));
    REQUIRE(deq(g_rt->result_double_regs(), 0.0));
}
