// test_int32_edge.cpp — 32-bit integer edge-case tests.
//
// Goals:
//   1. Carry propagation between low and high 16-bit words.
//   2. Values with significant high-word content (not just low-16).
//   3. Sign-boundary values (INT32_MIN, INT32_MAX).
//   4. Verify the quotient/remainder relationship q*d+r=n for div & mod.
//   5. Power-of-two and near-boundary alignment cases.
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"

// ---------------------------------------------------------------------------
// __mullong — carry between words
// ---------------------------------------------------------------------------

TEST(mullong_carry_low_to_high)
{
    // 0x0001FFFE * 2 = 0x0003FFFC (carry from bit 16)
    REQUIRE(g_rt->call32(rt_sym::mul32, 0x0001FFFE, 2));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x0003FFFC);
}

TEST(mullong_carry_high_word)
{
    // 0x00010000 * 0x00010000 → 0x100000000 wraps to 0
    REQUIRE(g_rt->call32(rt_sym::mul32, 0x00010000, 0x00010000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(mullong_high_word_nonzero_both_args)
{
    // 0x00020003 * 4 = 0x0008000C
    REQUIRE(g_rt->call32(rt_sym::mul32, 0x00020003, 4));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x0008000C);
}

TEST(mullong_signed_min_times_minus1)
{
    // INT32_MIN * -1 = INT32_MIN (wraps in 32-bit two's complement)
    REQUIRE(g_rt->call32(rt_sym::mul32, (uint32_t)0x80000000, (uint32_t)(-1)));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x80000000);
}

TEST(mullong_minus1_times_minus1)
{
    // (-1) * (-1) = 1
    REQUIRE(g_rt->call32(rt_sym::mul32, (uint32_t)(-1), (uint32_t)(-1)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1);
}

TEST(mullong_high_word_in_a)
{
    // 0x00FF0000 * 2 = 0x01FE0000
    REQUIRE(g_rt->call32(rt_sym::mul32, 0x00FF0000, 2));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x01FE0000);
}

TEST(mullong_modular_products_and_preserved_state)
{
    // Exercise all four partial-product limbs, including carries and both
    // sign bits. The helper's contract is the low 32 bits even when a signed
    // source expression would overflow; use unsigned host arithmetic here.
    const uint32_t edges[] = {
        0, 1, 0xff, 0x100, 0xffff, 0x10000, 0x10001,
        0x7fffffff, 0x80000000, 0xffff0000, 0xffffffff,
        0x13579bdf, 0xeca86420
    };
    uint32_t state = 0x62f79a35;
    for (unsigned i = 0; i < 2048; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        uint32_t a = i < 169 ? edges[i / 13] : state;
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        uint32_t b = i < 169 ? edges[i % 13] : state;

        uint16_t sp = g_rt->push32_arg(STACK_BASE, b);
        sp = g_rt->push16(sp, HALT_ADDR);
        xz80::cpu_state s{};
        s.de = static_cast<uint16_t>(a);
        s.hl = static_cast<uint16_t>(a >> 16);
        s.ix = 0x69c3;
        s.iy = 0x96b7;
        s.af2 = 0x53a9;
        s.bc2 = 0xc62d;
        s.de2 = 0xe817;
        s.hl2 = 0x4bf0;
        s.sp = sp;
        s.pc = rt_sym::mul32;
        g_rt->cpu.restore(s);
        for (unsigned step = 0; step < MAX_STEPS && !g_rt->cpu.halted(); ++step)
            g_rt->cpu.step();
        REQUIRE(g_rt->cpu.halted());
        REQUIRE_EQ(g_rt->result32(), a * b);
        const auto after = g_rt->snap();
        REQUIRE_EQ(after.sp, STACK_BASE - 4);
        REQUIRE_EQ(after.ix, s.ix);
        REQUIRE_EQ(after.iy, s.iy);
        REQUIRE_EQ(after.af2, s.af2);
        REQUIRE_EQ(after.bc2, s.bc2);
        REQUIRE_EQ(after.de2, s.de2);
        REQUIRE_EQ(after.hl2, s.hl2);
        for (unsigned byte = 0; byte < 4; ++byte)
            REQUIRE_EQ(g_rt->mem.read(STACK_BASE - 4 + byte),
                       static_cast<uint8_t>(b >> (byte * 8)));
    }
}

// ---------------------------------------------------------------------------
// __divulong — values with high-word bits
// ---------------------------------------------------------------------------

TEST(divulong_high_word_dividend)
{
    // 0x00020000 / 2 = 0x00010000
    REQUIRE(g_rt->call32(rt_sym::div32, 0x00020000, 2));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x00010000);
}

TEST(divulong_high_word_divisor)
{
    // 0x00200000 / 0x00100000 = 2, rem 0
    REQUIRE(g_rt->call32(rt_sym::div32, 0x00200000, 0x00100000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)2);
}

TEST(divulong_crossing_16bit_boundary)
{
    // 0x000FFFFF / 0x1000 = 0xFF (255), rem 0xFFF
    REQUIRE(g_rt->call32(rt_sym::div32, 0x000FFFFF, 0x1000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)(0x000FFFFF / 0x1000));
}

TEST(divulong_full_32bit_dividend)
{
    // 0xFFFFFFFF / 0x10000 = 0xFFFF, rem 0xFFFF
    REQUIRE(g_rt->call32(rt_sym::div32, 0xFFFFFFFF, 0x10000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0xFFFF);
}

TEST(divulong_quotient_remainder_verify)
{
    // Verify: q * divisor + remainder == dividend for a random-ish case
    uint32_t dividend = 1234567891u;
    uint32_t divisor  = 99991u;
    REQUIRE(g_rt->call32(rt_sym::div32, dividend, divisor));
    uint32_t q = g_rt->result32();
    REQUIRE(g_rt->call32(rt_sym::mod32, dividend, divisor));
    uint32_t r = g_rt->result32();
    REQUIRE_EQ(q * divisor + r, dividend);
}

// ---------------------------------------------------------------------------
// __divslong — sign-boundary values
// ---------------------------------------------------------------------------

TEST(divslong_int32_max_by_1)
{
    // INT32_MAX / 1 = INT32_MAX
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)0x7FFFFFFF, 1));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0x7FFFFFFF);
}

TEST(divslong_int32_min_by_1)
{
    // INT32_MIN / 1 = INT32_MIN
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)0x80000000, 1));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0x80000000);
}

TEST(divslong_int32_max_by_int32_max)
{
    // INT32_MAX / INT32_MAX = 1
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1);
}

TEST(divslong_neg_high_word)
{
    // -0x00010001 / 0x10001 = -1, (negative value with both words set)
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)(-(int32_t)0x10001), 0x10001));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

TEST(divslong_quotient_remainder_verify_signed)
{
    // Verify C semantics: q*d + r == n, sign(r) == sign(n)
    int32_t dividend = -1234567;
    int32_t divisor  = 777;
    REQUIRE(g_rt->call32(rt_sym::sdiv32, (uint32_t)dividend, (uint32_t)divisor));
    int32_t q = (int32_t)g_rt->result32();
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)dividend, (uint32_t)divisor));
    int32_t r = (int32_t)g_rt->result32();
    REQUIRE_EQ(q * divisor + r, dividend);
    // C11: sign(r) == sign(dividend) when both non-zero
    REQUIRE((r == 0) || ((r < 0) == (dividend < 0)));
}

// ---------------------------------------------------------------------------
// __modulong — high-word values
// ---------------------------------------------------------------------------

TEST(modulong_high_word_dividend)
{
    // 0x00010001 % 0x10000 = 1
    REQUIRE(g_rt->call32(rt_sym::mod32, 0x00010001, 0x10000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)1);
}

TEST(modulong_high_word_both)
{
    // 0x00030007 % 0x00020000 = 0x00010007
    REQUIRE(g_rt->call32(rt_sym::mod32, 0x00030007, 0x00020000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x00010007);
}

TEST(modulong_max_mod_1)
{
    // 0xFFFFFFFF % 1 = 0
    REQUIRE(g_rt->call32(rt_sym::mod32, 0xFFFFFFFF, 1));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(modulong_max_mod_max)
{
    // 0xFFFFFFFF % 0xFFFFFFFF = 0
    REQUIRE(g_rt->call32(rt_sym::mod32, 0xFFFFFFFF, 0xFFFFFFFF));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

// ---------------------------------------------------------------------------
// __modslong — sign boundary values
// ---------------------------------------------------------------------------

TEST(modslong_int32_min_mod_large_pos)
{
    // INT32_MIN % (INT32_MAX) = -1  (since INT32_MIN = -1 * INT32_MAX + (-1))
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)0x80000000, (uint32_t)0x7FFFFFFF));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

TEST(modslong_neg_mod_1)
{
    // Any value % 1 = 0
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)(-9999999), 1));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0);
}

TEST(modslong_sign_preserved_neg_divisor)
{
    // -101 % -10 = -1 (sign of dividend)
    REQUIRE(g_rt->call32(rt_sym::smod32, (uint32_t)(-101), (uint32_t)(-10)));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

// ---------------------------------------------------------------------------
// ___mulsint2slong / ___muluint2ulong — carry into high word
// ---------------------------------------------------------------------------

TEST(mulsint2slong_carry_into_high)
{
    // 0x4000 * 4 = 0x10000 — carry from low into high word
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, 0x4000, 4));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0x10000);
}

TEST(mulsint2slong_identity)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, 12345, 1));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)12345);
}

TEST(mulsint2slong_zero)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::mulsint2slong, 0, 32767));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0);
}

TEST(muluint2ulong_carry_into_high)
{
    // 0x8000 * 2 = 0x10000 — carry
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 0x8000, 2));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x10000);
}

TEST(muluint2ulong_identity)
{
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 12345, 1));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)12345);
}

TEST(muluint2ulong_half_max)
{
    // 0x8000 * 0x8000 = 0x40000000
    REQUIRE(g_rt->call16x16to32(rt_sym::muluint2ulong, 0x8000, 0x8000));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0x40000000);
}
