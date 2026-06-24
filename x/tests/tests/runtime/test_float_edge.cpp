// test_float_edge.cpp — float edge-case tests.
//
// Goals:
//   1. Exponent arithmetic: pure exponent add/subtract, overflow region.
//   2. Alignment-shift extremes: when one operand is negligible vs the other.
//   3. Exact cancellation (equal magnitudes, opposite signs).
//   4. Equal-exponent additions (forces the exponent bump path in fsadd).
//   5. Sign table coverage: all four ±/± combinations for mul/div.
//   6. Round-trip conversions: int → float → int must be lossless for
//      values that are exactly representable (small integers).
//   7. fsneg on special bit patterns.
//   8. fscmp sign-asymmetry: deeper negative-number ordering.
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"
#include "float_helpers.hpp"
#include <cmath>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static uint32_t float_bits(float f)
{
    uint32_t b; std::memcpy(&b, &f, 4); return b;
}

// ---------------------------------------------------------------------------
// fsadd — exponent edge cases
// ---------------------------------------------------------------------------

TEST(fsadd_equal_exponents_same_sign_bump)
{
    // 1.0 + 1.0 = 2.0: equal exponents, addition bumps the exponent
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1.0f, 1.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 2.0f));
}

TEST(fsadd_equal_exponents_larger_sum)
{
    // 1.5 + 1.5 = 3.0
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1.5f, 1.5f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.0f));
}

TEST(fsadd_very_different_magnitudes)
{
    // Adding a tiny value to a large one: tiny disappears into noise
    // (the alignment shift right-shifts the small mantissa 31+ positions)
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1e10f, 1e-10f));
    REQUIRE(feq(g_rt->result_float_hlde(), 1e10f, 1e-4f));
}

TEST(fsadd_shift_exactly_31)
{
    // Force the shift-count >= 31 guard: result should equal the large operand
    float large = std::ldexp(1.0f, 20);  // 2^20 = 1048576
    float tiny  = std::ldexp(1.0f, -15); // 2^-15 ≈ 3e-5; exponent diff = 35
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, large, tiny));
    REQUIRE(feq(g_rt->result_float_hlde(), large, 1e-5f));
}

TEST(fsadd_exact_cancel_equal_magnitude)
{
    // 16384.0 + (-16384.0) = 0.0
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 16384.0f, -16384.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(fsadd_chain_of_adds)
{
    // 1 + 2 + 4 + 8 = 15 (verify no accumulation error)
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1.0f, 2.0f));
    float s = g_rt->result_float_hlde();
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, s, 4.0f));
    s = g_rt->result_float_hlde();
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, s, 8.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 15.0f));
}

// ---------------------------------------------------------------------------
// fssub — more edge cases
// ---------------------------------------------------------------------------

TEST(fssub_equal_values_cancel)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, 12345.0f, 12345.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(fssub_negative_from_negative)
{
    // (-10) - (-3) = -7
    REQUIRE(g_rt->call_float2(rt_sym::fssub, -10.0f, -3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -7.0f));
}

// ---------------------------------------------------------------------------
// fsmul — exponent addition, sign table, powers of two
// ---------------------------------------------------------------------------

TEST(fsmul_pure_exponent_add)
{
    // 4.0 * 8.0 = 32.0 (exponents add: 2^2 * 2^3 = 2^5)
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 4.0f, 8.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 32.0f));
}

TEST(fsmul_fractional_result)
{
    // 0.25 * 0.25 = 0.0625 (exponent subtraction)
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 0.25f, 0.25f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0625f));
}

TEST(fsmul_power_of_two_chain)
{
    // 2^10 * 2^10 = 2^20 = 1048576
    float p10 = 1024.0f;
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, p10, p10));
    REQUIRE(feq(g_rt->result_float_hlde(), 1048576.0f));
}

TEST(fsmul_sign_pp)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul,  5.0f,  3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(),  15.0f));
}

TEST(fsmul_sign_pn)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul,  5.0f, -3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -15.0f));
}

TEST(fsmul_sign_np)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, -5.0f,  3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -15.0f));
}

TEST(fsmul_sign_nn)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, -5.0f, -3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(),  15.0f));
}

// ---------------------------------------------------------------------------
// fsdiv — exponent subtraction, identity, sign table
// ---------------------------------------------------------------------------

TEST(fsdiv_pure_exponent_sub)
{
    // 32.0 / 8.0 = 4.0 (exponents subtract)
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 32.0f, 8.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 4.0f));
}

TEST(fsdiv_by_self)
{
    // x / x = 1.0 for non-zero x
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 7.5f, 7.5f));
    REQUIRE(feq(g_rt->result_float_hlde(), 1.0f));
}

TEST(fsdiv_halving_chain)
{
    // 1024 / 2 = 512; 512 / 2 = 256; 256 / 2 = 128
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 1024.0f, 2.0f));
    float v = g_rt->result_float_hlde();
    REQUIRE(feq(v, 512.0f));
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, v, 2.0f));
    v = g_rt->result_float_hlde();
    REQUIRE(feq(v, 256.0f));
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, v, 2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 128.0f));
}

TEST(fsdiv_sign_pp)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv,  9.0f,  3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(),  3.0f));
}

TEST(fsdiv_sign_pn)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv,  9.0f, -3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -3.0f));
}

TEST(fsdiv_sign_np)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, -9.0f,  3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -3.0f));
}

TEST(fsdiv_sign_nn)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, -9.0f, -3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(),  3.0f));
}

// ---------------------------------------------------------------------------
// fscmp — negative-number ordering
// ---------------------------------------------------------------------------

TEST(fscmp_neg_ordering)
{
    // -5 < -1: less-negative is GREATER
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, -5.0f, -1.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));

    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, -1.0f, -5.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(1));
}

TEST(fscmp_symmetry)
{
    // fscmp(a,b) and fscmp(b,a) should have opposite signs
    float pairs[][2] = {
        {1.0f, 2.0f}, {-3.0f, 3.0f}, {100.5f, 0.001f}
    };
    for (auto& p : pairs) {
        REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, p[0], p[1]));
        int16_t fwd = g_rt->result_de_s16();
        REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, p[1], p[0]));
        int16_t rev = g_rt->result_de_s16();
        REQUIRE_EQ(fwd, (int16_t)(-rev));
    }
}

TEST(fscmp_transitivity)
{
    // a < b < c: fscmp(a,b)<0, fscmp(b,c)<0, fscmp(a,c)<0
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 1.0f, 2.0f));
    REQUIRE((int16_t)g_rt->snap().de < 0);
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 2.0f, 3.0f));
    REQUIRE((int16_t)g_rt->snap().de < 0);
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 1.0f, 3.0f));
    REQUIRE((int16_t)g_rt->snap().de < 0);
}

// ---------------------------------------------------------------------------
// fsneg — bit-level correctness
// ---------------------------------------------------------------------------

TEST(fsneg_bit_pattern_1_0)
{
    // 1.0f = 0x3F800000; -1.0f = 0xBF800000
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 1.0f));
    REQUIRE_EQ(float_bits(g_rt->result_float_dehl()), (uint32_t)0xBF800000);
}

TEST(fsneg_bit_pattern_neg_1)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, -1.0f));
    REQUIRE_EQ(float_bits(g_rt->result_float_dehl()), (uint32_t)0x3F800000);
}

TEST(fsneg_double_negation)
{
    // -(-x) = x for any finite x
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 3.14159f));
    float neg = g_rt->result_float_dehl();
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, neg));
    REQUIRE(feq(g_rt->result_float_dehl(), 3.14159f));
}

TEST(fsneg_zero)
{
    // -0.0f = sign bit flipped, but we get the bit pattern right
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 0.0f));
    uint32_t bits = float_bits(g_rt->result_float_dehl());
    REQUIRE_EQ(bits, (uint32_t)0x80000000); // -0.0f
}

TEST(fsneg_large_exponent)
{
    float x = 1.5e20f;
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, x));
    REQUIRE(feq(g_rt->result_float_dehl(), -x));
}

// ---------------------------------------------------------------------------
// Round-trip conversions: int → float → int
// ---------------------------------------------------------------------------

TEST(roundtrip_uint16_small)
{
    // All small uint16 values round-trip exactly
    uint16_t vals[] = {0, 1, 2, 7, 127, 128, 255, 256, 1000, 32767, 65535};
    for (uint16_t v : vals) {
        REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, v));
        float f = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float1(rt_sym::fs2uint, f));
        REQUIRE_EQ(g_rt->snap().de, (uint16_t)v);
    }
}

TEST(roundtrip_sint16_positive)
{
    int16_t vals[] = {0, 1, 100, 127, 128, 1000, 32767};
    for (int16_t v : vals) {
        REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, (uint16_t)v));
        float f = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float1(rt_sym::fs2sint, f));
        REQUIRE_EQ((int16_t)g_rt->snap().de, v);
    }
}

TEST(roundtrip_sint16_negative)
{
    int16_t vals[] = {-1, -100, -128, -1000, -32768};
    for (int16_t v : vals) {
        REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, (uint16_t)v));
        float f = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float1(rt_sym::fs2sint, f));
        REQUIRE_EQ((int16_t)g_rt->snap().de, v);
    }
}

TEST(roundtrip_uint32_small)
{
    uint32_t vals[] = {0, 1, 1000, 65536, 1000000, 16777215 /* 2^24-1, exact */};
    for (uint32_t v : vals) {
        REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, v));
        float f = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float1(rt_sym::fs2ulong, f));
        REQUIRE_EQ(g_rt->result32(), v);
    }
}

TEST(roundtrip_sint32_small)
{
    int32_t vals[] = {0, 1, -1, 1000, -1000, 16777215, -16777215};
    for (int32_t v : vals) {
        REQUIRE(g_rt->call_long_to_float(rt_sym::slong2fs, (uint32_t)v));
        float f = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float1(rt_sym::fs2slong, f));
        REQUIRE_EQ((int32_t)g_rt->result32(), v);
    }
}

// ---------------------------------------------------------------------------
// fsadd / fsmul consistency: a*2 == a+a
// ---------------------------------------------------------------------------

TEST(fsmul_vs_double_add)
{
    float vals[] = {1.0f, 3.5f, 100.0f, 0.125f, -7.25f};
    for (float v : vals) {
        REQUIRE(g_rt->call_float2(rt_sym::fsmul, v, 2.0f));
        float by_mul = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float2(rt_sym::fsadd, v, v));
        float by_add = g_rt->result_float_hlde();
        REQUIRE(feq(by_mul, by_add));
    }
}

TEST(fsdiv_vs_half_mul)
{
    // a / 2 == a * 0.5
    float vals[] = {4.0f, 100.0f, -64.0f, 1.5f};
    for (float v : vals) {
        REQUIRE(g_rt->call_float2(rt_sym::fsdiv, v, 2.0f));
        float by_div = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float2(rt_sym::fsmul, v, 0.5f));
        float by_mul = g_rt->result_float_hlde();
        REQUIRE(feq(by_div, by_mul));
    }
}

// ---------------------------------------------------------------------------
// Float subtraction implemented as negation + addition
// fssub flips b's sign bit then tail-calls fsadd — verify consistency
// ---------------------------------------------------------------------------

TEST(fssub_vs_neg_add)
{
    // a - b  ==  a + (-b)
    float pairs[][2] = {
        {5.0f, 3.0f}, {-1.0f, 4.0f}, {0.5f, 0.75f}, {100.0f, 200.0f}
    };
    for (auto& p : pairs) {
        REQUIRE(g_rt->call_float2(rt_sym::fssub, p[0], p[1]));
        float by_sub = g_rt->result_float_hlde();
        REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, p[1]));
        float neg_b = g_rt->result_float_dehl();
        REQUIRE(g_rt->call_float2(rt_sym::fsadd, p[0], neg_b));
        float by_neg_add = g_rt->result_float_hlde();
        REQUIRE(feq(by_sub, by_neg_add));
    }
}
