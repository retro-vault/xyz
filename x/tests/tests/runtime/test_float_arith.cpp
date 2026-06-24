// test_float_arith.cpp — float arithmetic runtime function tests.
//
// Covers: __fsadd/___fsadd, __fssub/___fssub, __fsmul/___fsmul,
//         __fsdiv/___fsdiv, ___fscmp, ___fseq, ___fslt, __fsneg,
//         __fssqrt (stub), __fsatan2 (stub)
//
// Float ABI: first arg in HL:DE (HL=high, DE=low), second on stack.
// Result in HL:DE (HL=high, DE=low).
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"
#include "float_helpers.hpp"

// ---------------------------------------------------------------------------
// __fsadd — float add
// ---------------------------------------------------------------------------

TEST(fsadd_one_plus_two)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1.0f, 2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.0f));
}

TEST(fsadd_identity_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 5.0f, 0.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 5.0f));
}

TEST(fsadd_fractions)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1.5f, 2.5f));
    REQUIRE(feq(g_rt->result_float_hlde(), 4.0f));
}

TEST(fsadd_cancel_to_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 7.0f, -7.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(fsadd_neg_plus_neg)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, -3.0f, -4.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -7.0f));
}

TEST(fsadd_large_values)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1e10f, 1e10f));
    REQUIRE(feq(g_rt->result_float_hlde(), 2e10f));
}

TEST(fsadd_small_plus_large)
{
    // Result should be dominated by large value
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 1000.0f, 0.001f));
    REQUIRE(feq(g_rt->result_float_hlde(), 1000.001f, 1e-4f));
}

TEST(fsadd_both_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsadd, 0.0f, 0.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

// ---------------------------------------------------------------------------
// __fssub — float subtract
// ---------------------------------------------------------------------------

TEST(fssub_basic)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, 5.0f, 3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 2.0f));
}

TEST(fssub_equals_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, 1.0f, 1.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(fssub_negative_result)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, 3.0f, 5.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -2.0f));
}

TEST(fssub_neg_minus_pos)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, -3.0f, 2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -5.0f));
}

TEST(fssub_zero_minus_n)
{
    REQUIRE(g_rt->call_float2(rt_sym::fssub, 0.0f, 4.5f));
    REQUIRE(feq(g_rt->result_float_hlde(), -4.5f));
}

// ---------------------------------------------------------------------------
// __fsmul — float multiply
// ---------------------------------------------------------------------------

TEST(fsmul_two_by_three)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 2.0f, 3.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 6.0f));
}

TEST(fsmul_by_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 12345.0f, 0.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(fsmul_neg_times_pos)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, -4.0f, 5.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -20.0f));
}

TEST(fsmul_neg_times_neg)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, -3.0f, -4.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 12.0f));
}

TEST(fsmul_fractions)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 0.5f, 0.5f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.25f));
}

TEST(fsmul_identity)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 3.14159f, 1.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.14159f));
}

TEST(fsmul_large)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsmul, 1000.0f, 1000.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 1000000.0f));
}

// ---------------------------------------------------------------------------
// __fsdiv — float divide
// ---------------------------------------------------------------------------

TEST(fsdiv_six_by_two)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 6.0f, 2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.0f));
}

TEST(fsdiv_neg_dividend)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, -6.0f, 2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), -3.0f));
}

TEST(fsdiv_both_neg)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, -6.0f, -2.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.0f));
}

TEST(fsdiv_fraction)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 1.0f, 4.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.25f));
}

TEST(fsdiv_identity)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 3.14159f, 1.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 3.14159f));
}

TEST(fsdiv_large_numerator)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsdiv, 1e9f, 1e6f));
    REQUIRE(feq(g_rt->result_float_hlde(), 1e3f));
}

// ---------------------------------------------------------------------------
// ___fscmp — float compare: returns -1, 0, +1 in DE
// ---------------------------------------------------------------------------

TEST(fscmp_less_than)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 1.0f, 2.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
}

TEST(fscmp_greater_than)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 2.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)1);
}

TEST(fscmp_equal)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 3.14f, 3.14f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)0);
}

TEST(fscmp_neg_less_than_pos)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, -1.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
}

TEST(fscmp_zero_vs_positive)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 0.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
}

TEST(fscmp_positive_vs_zero)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, 1.0f, 0.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)1);
}

TEST(fscmp_both_negative)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fscmp, -2.0f, -1.0f));
    REQUIRE_EQ(g_rt->result_de_s16(), (int16_t)(-1));
}

// ---------------------------------------------------------------------------
// ___fseq — float equal: returns 1 in A if a==b, else 0
// ---------------------------------------------------------------------------

TEST(fseq_equal)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fseq, 3.14f, 3.14f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

TEST(fseq_not_equal)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fseq, 1.0f, 2.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

TEST(fseq_zeros)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fseq, 0.0f, 0.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

TEST(fseq_neg_and_pos)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fseq, -1.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

// ---------------------------------------------------------------------------
// ___fslt — float less-than: returns 1 in A if a<b, else 0
// ---------------------------------------------------------------------------

TEST(fslt_less)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fslt, 1.0f, 2.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

TEST(fslt_greater)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fslt, 2.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

TEST(fslt_equal)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fslt, 1.0f, 1.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

TEST(fslt_neg_less_pos)
{
    REQUIRE(g_rt->call_float_cmp(rt_sym::fslt, -5.0f, 0.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)1);
}

// ---------------------------------------------------------------------------
// __fsneg — float negate: arg entirely on stack
// Result in DE:HL (DE=high, HL=low) — use result_float_dehl()
// ---------------------------------------------------------------------------

TEST(fsneg_positive)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 3.0f));
    REQUIRE(feq(g_rt->result_float_dehl(), -3.0f));
}

TEST(fsneg_negative)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, -3.0f));
    REQUIRE(feq(g_rt->result_float_dehl(), 3.0f));
}

TEST(fsneg_one)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 1.0f));
    REQUIRE(feq(g_rt->result_float_dehl(), -1.0f));
}

TEST(fsneg_minus_one)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, -1.0f));
    REQUIRE(feq(g_rt->result_float_dehl(), 1.0f));
}

TEST(fsneg_large_value)
{
    REQUIRE(g_rt->call_float_stack(rt_sym::fsneg, 1e15f));
    REQUIRE(feq(g_rt->result_float_dehl(), -1e15f));
}

// ---------------------------------------------------------------------------
// __fssqrt — stub: always returns 0.0f
// ---------------------------------------------------------------------------

TEST(fssqrt_stub_returns_zero)
{
    // The current implementation is a stub that returns 0.0f
    REQUIRE(g_rt->call_float1(rt_sym::fssqrt, 4.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

// ---------------------------------------------------------------------------
// __fsatan2 — stub: always returns 0.0f
// ---------------------------------------------------------------------------

TEST(fsatan2_stub_returns_zero)
{
    REQUIRE(g_rt->call_float2(rt_sym::fsatan2, 1.0f, 1.0f));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}
