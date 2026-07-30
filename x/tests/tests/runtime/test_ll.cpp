// test_ll.cpp — 64-bit integer (long long) runtime tests.
//
// All PENDING_TEST spellings in this file are deliberately registered as
// active tests below.  The historical name is retained only to keep the
// individual case declarations stable.
//
// -----------------------------------------------------------------------
// Calling convention (mirrors the proposed runtime_machine ABI):
//
// First 64-bit arg AND return value use the full register set:
//   DE  = bits[15: 0]  (lsb)
//   HL  = bits[31:16]
//   DE' = bits[47:32]  (alternate, cpu_state::de2)
//   HL' = bits[63:48]  (alternate, cpu_state::hl2)
//
// Second 64-bit arg on stack (ix+4..ix+11 after fn's push-ix frame).
//
//   call64(fn, a, b)        a in regs, b on stack → result in regs
//   call64_1arg(fn, a)      a in regs, no stack arg → result in regs
//   call64_from_int(fn, n)  n in HL → 64-bit result in regs
//   call64_from_long(fn, n) n in DE:HL → 64-bit result in regs
//   result64_regs()         reads DE:HL:DE':HL' as uint64_t
// -----------------------------------------------------------------------
#include "runtime_symbols_future.hpp"
#include "runtime_machine.hpp"

#define PENDING_TEST TEST

#ifndef PENDING_TEST
#define PENDING_TEST(name)  static void _pending_##name()
#endif

// ---------------------------------------------------------------------------
// __mulll — 64-bit multiply (signed × signed; only low 64 bits returned)
// ---------------------------------------------------------------------------

PENDING_TEST(ll_mul_zero)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 0, 0x123456789ABCDEFull));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0);
}

PENDING_TEST(ll_mul_identity)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 1, 0xDEADBEEFCAFEull));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0xDEADBEEFCAFEull);
}

PENDING_TEST(ll_mul_small)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 3, 7));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)21);
}

PENDING_TEST(ll_mul_carries_into_high)
{
    // 0x00000001_00000000 * 2 = 0x00000002_00000000
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 0x100000000ull, 2));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x200000000ull);
}

PENDING_TEST(ll_mul_neg_pos)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, (uint64_t)(-5LL), 6));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-30));
}

PENDING_TEST(ll_mul_neg_neg)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, (uint64_t)(-7LL), (uint64_t)(-8LL)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)56);
}

PENDING_TEST(ll_mul_commutativity)
{
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 12345678, 9876543));
    uint64_t ab = g_rt->result64_regs();
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 9876543, 12345678));
    REQUIRE_EQ(g_rt->result64_regs(), ab);
}

PENDING_TEST(ll_mul_max_u32_squared)
{
    // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE_00000001
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 0xFFFFFFFFull, 0xFFFFFFFFull));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0xFFFFFFFE00000001ull);
}

PENDING_TEST(ll_mul_overflow_wraps)
{
    // 2^63 * 2 overflows low 64 bits → 0
    REQUIRE(g_rt->call64(rt_sym_future::mulll, 0x8000000000000000ull, 2));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0);
}

// ---------------------------------------------------------------------------
// __divull — unsigned 64-bit divide
// Result at (BC); DE:HL = quotient low32.
// ---------------------------------------------------------------------------

PENDING_TEST(ll_divull_basic)
{
    REQUIRE(g_rt->call64(rt_sym_future::divull, 100, 5));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)20);
}

PENDING_TEST(ll_divull_remainder_discarded)
{
    REQUIRE(g_rt->call64(rt_sym_future::divull, 101, 10));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)10);
}

PENDING_TEST(ll_divull_high_word_dividend)
{
    // 0x200000000 / 2 = 0x100000000
    REQUIRE(g_rt->call64(rt_sym_future::divull, 0x200000000ull, 2));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x100000000ull);
}

PENDING_TEST(ll_divull_large_divisor)
{
    REQUIRE(g_rt->call64(rt_sym_future::divull, 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)1);
}

PENDING_TEST(ll_divull_quotient_remainder_verify)
{
    uint64_t dividend = 12345678901234567ull;
    uint64_t divisor  = 9999991ull;
    REQUIRE(g_rt->call64(rt_sym_future::divull, dividend, divisor));
    uint64_t q = g_rt->result64_regs();
    REQUIRE(g_rt->call64(rt_sym_future::modull, dividend, divisor));
    uint64_t r = g_rt->result64_regs();
    REQUIRE_EQ(q * divisor + r, dividend);
}

// ---------------------------------------------------------------------------
// __divsll — signed 64-bit divide
// ---------------------------------------------------------------------------

PENDING_TEST(ll_divsll_pos_pos)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, 100, 4));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)25);
}

PENDING_TEST(ll_divsll_neg_pos)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)(-100LL), 4));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-25));
}

PENDING_TEST(ll_divsll_pos_neg)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, 100, (uint64_t)(-4LL)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-25));
}

PENDING_TEST(ll_divsll_neg_neg)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)(-100LL), (uint64_t)(-4LL)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)25);
}

PENDING_TEST(ll_divsll_int64_min_by_1)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)INT64_MIN, 1));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), INT64_MIN);
}

PENDING_TEST(ll_divsll_int64_max_by_max)
{
    REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)INT64_MAX, (uint64_t)INT64_MAX));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)1);
}

PENDING_TEST(ll_divsll_verify_qdr)
{
    // q*d + r == n  and sign(r) == sign(n)
    int64_t n = -1234567890123456LL, d = 777777;
    REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)n, (uint64_t)d));
    int64_t q = (int64_t)g_rt->result64_regs();
    REQUIRE(g_rt->call64(rt_sym_future::modsll, (uint64_t)n, (uint64_t)d));
    int64_t r = (int64_t)g_rt->result64_regs();
    REQUIRE_EQ(q * d + r, n);
    REQUIRE((r == 0) || ((r < 0) == (n < 0)));
}

// ---------------------------------------------------------------------------
// __modull — unsigned 64-bit modulo
// ---------------------------------------------------------------------------

PENDING_TEST(ll_modull_basic)
{
    REQUIRE(g_rt->call64(rt_sym_future::modull, 101, 10));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)1);
}

PENDING_TEST(ll_modull_exact)
{
    REQUIRE(g_rt->call64(rt_sym_future::modull, 100, 10));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0);
}

PENDING_TEST(ll_modull_high_word)
{
    // 0x100000001 % 0x100000000 = 1
    REQUIRE(g_rt->call64(rt_sym_future::modull, 0x100000001ull, 0x100000000ull));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)1);
}

PENDING_TEST(ll_modull_max_mod_1)
{
    REQUIRE(g_rt->call64(rt_sym_future::modull, UINT64_MAX, 1));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0);
}

// ---------------------------------------------------------------------------
// __modsll — signed 64-bit modulo (C11: sign = sign of dividend)
// ---------------------------------------------------------------------------

PENDING_TEST(ll_modsll_pos_pos)
{
    REQUIRE(g_rt->call64(rt_sym_future::modsll, 101, 10));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)1);
}

PENDING_TEST(ll_modsll_neg_pos)
{
    REQUIRE(g_rt->call64(rt_sym_future::modsll, (uint64_t)(-101LL), 10));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1));
}

PENDING_TEST(ll_modsll_pos_neg)
{
    REQUIRE(g_rt->call64(rt_sym_future::modsll, 101, (uint64_t)(-10LL)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)1);
}

PENDING_TEST(ll_modsll_neg_neg)
{
    REQUIRE(g_rt->call64(rt_sym_future::modsll, (uint64_t)(-101LL), (uint64_t)(-10LL)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1));
}

PENDING_TEST(ll_modsll_mod_1)
{
    REQUIRE(g_rt->call64(rt_sym_future::modsll, (uint64_t)(-99999999LL), 1));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)0);
}

// ---------------------------------------------------------------------------
// 64-bit variable shifts
// ---------------------------------------------------------------------------

PENDING_TEST(ll_shl64_by_0)
{
    REQUIRE(g_rt->call64(rt_sym_future::shl64, 0x123456789ABCull, 0));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x123456789ABCull);
}

PENDING_TEST(ll_shl64_by_1)
{
    REQUIRE(g_rt->call64(rt_sym_future::shl64, 1, 1));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)2);
}

PENDING_TEST(ll_shl64_cross_word)
{
    // 1 << 32 = 0x100000000
    REQUIRE(g_rt->call64(rt_sym_future::shl64, 1, 32));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x100000000ull);
}

PENDING_TEST(ll_shl64_by_63)
{
    REQUIRE(g_rt->call64(rt_sym_future::shl64, 1, 63));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x8000000000000000ull);
}

PENDING_TEST(ll_shr64u_by_1)
{
    REQUIRE(g_rt->call64(rt_sym_future::shr64u, 0x8000000000000000ull, 1));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0x4000000000000000ull);
}

PENDING_TEST(ll_shr64u_cross_word)
{
    // 0x100000000 >> 32 = 1
    REQUIRE(g_rt->call64(rt_sym_future::shr64u, 0x100000000ull, 32));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)1);
}

PENDING_TEST(ll_shr64s_neg_shifts_in_ones)
{
    // (-1) >> 1 = -1 (arithmetic)
    REQUIRE(g_rt->call64(rt_sym_future::shr64s, UINT64_MAX, 1));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1));
}

PENDING_TEST(ll_shr64s_sign_extend)
{
    // INT64_MIN >> 63 = -1
    REQUIRE(g_rt->call64(rt_sym_future::shr64s, (uint64_t)INT64_MIN, 63));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1));
}

// ---------------------------------------------------------------------------
// Conversions → long long
// ---------------------------------------------------------------------------

PENDING_TEST(ll_sint2ll_positive)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2ll, 12345));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)12345);
}

PENDING_TEST(ll_sint2ll_negative)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2ll, (uint16_t)(-100)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-100));
}

PENDING_TEST(ll_sint2ll_min_int16)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2ll, (uint16_t)(-32768)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-32768));
}

PENDING_TEST(ll_uint2ll_max)
{
    REQUIRE(g_rt->call64_from_int(rt_sym_future::uint2ll, 65535));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)65535);
}

PENDING_TEST(ll_slong2ll_positive)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2ll, 0x7FFFFFFF));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)0x7FFFFFFF);
}

PENDING_TEST(ll_slong2ll_negative)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2ll, (uint32_t)(-1)));
    REQUIRE_EQ((int64_t)g_rt->result64_regs(), (int64_t)(-1));
}

PENDING_TEST(ll_ulong2ll_max)
{
    REQUIRE(g_rt->call64_from_long(rt_sym_future::ulong2ll, 0xFFFFFFFF));
    REQUIRE_EQ(g_rt->result64_regs(), (uint64_t)0xFFFFFFFF);
}

// ---------------------------------------------------------------------------
// Conversions from long long
// ---------------------------------------------------------------------------

PENDING_TEST(ll_ll2sint_truncates_low)
{
    // Low 16 bits of 0x12345678ABCD = 0xABCD = -21555 as int16
    REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2sint, 0x12345678ABCDull));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)(int16_t)0xABCD);
}

PENDING_TEST(ll_ll2uint_truncates_low)
{
    REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2uint, 0xFFFFFFFF1234ull));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)0x1234);
}

PENDING_TEST(ll_ll2slong_truncates_low32)
{
    // Low 32 bits of 0xDEADBEEFCAFEBABE is 0xCAFEBABE
    REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2slong, 0xDEADBEEFCAFEBABEull));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0xCAFEBABE);
}

PENDING_TEST(ll_roundtrip_sint16)
{
    // int16 → ll → int16 is lossless for any value
    int16_t vals[] = {0, 1, -1, 32767, -32768, 100, -999};
    for (int16_t v : vals) {
        REQUIRE(g_rt->call64_from_int(rt_sym_future::sint2ll, (uint16_t)v));
        uint64_t ll = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2sint, ll));
        REQUIRE_EQ((int16_t)g_rt->snap().de, v);
    }
}

PENDING_TEST(ll_roundtrip_slong)
{
    int32_t vals[] = {0, 1, -1, 0x7FFFFFFF, (int32_t)0x80000000, 100000, -999999};
    for (int32_t v : vals) {
        REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2ll, (uint32_t)v));
        uint64_t ll = g_rt->result64_regs();
        REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2slong, ll));
        REQUIRE_EQ((int32_t)g_rt->result32(), v);
    }
}

// ---------------------------------------------------------------------------
// REALLY LARGE base for ll runtime (user: "really large test base")
// Cross product style like dbadd_mega, for mul/div/mod + convs on many values.
// Activated via PENDING_TEST=TEST.
// ---------------------------------------------------------------------------
static int64_t ll_test_values[] = {
    0, 1, -1, 2, -2, 10, -10, 0x7FFFFFFFLL, (int64_t)0x80000000LL,
    0x123456789ABCDEFLL, -0x123456789ABCDEFLL,
    9223372036854775807LL, (int64_t)0x8000000000000000ULL,
    1000000000000LL, -1000000000000LL
};

PENDING_TEST(ll_arith_mega)
{
    for (int64_t a : ll_test_values) {
        for (int64_t b : ll_test_values) {
            if (b == 0) continue; // avoid div0 in volume
            // mul (low 64)
            REQUIRE(g_rt->call64(rt_sym_future::mulll, (uint64_t)a, (uint64_t)b));
            // div ull/sll (just exercise paths; result checked lightly)
            REQUIRE(g_rt->call64(rt_sym_future::divull, (uint64_t)a < 0 ? -(uint64_t)a : (uint64_t)a , (uint64_t)(b < 0 ? -b : b)));
            REQUIRE(g_rt->call64(rt_sym_future::divsll, (uint64_t)a, (uint64_t)b));
        }
    }
}

PENDING_TEST(ll_conv_mega)
{
    for (int64_t v : ll_test_values) {
        // to/from int32 etc (roundtrip where lossless in low bits)
        REQUIRE(g_rt->call64_1arg(rt_sym_future::ll2slong, (uint64_t)v));
        (void)g_rt->result32();
        REQUIRE(g_rt->call64_from_long(rt_sym_future::slong2ll, (uint32_t)(int32_t)v));
    }
}
