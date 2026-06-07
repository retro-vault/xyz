// test_float_conv.cpp — float conversion runtime function tests.
//
// Covers:
//   int  → float : ___uint2fs, ___sint2fs, ___uchar2fs, ___schar2fs,
//                  ___ulong2fs, ___slong2fs
//   float → int  : ___fs2uint, ___fs2sint, ___fs2uchar, ___fs2schar,
//                  ___fs2ulong, ___fs2slong
//   stubs:         __fstoi, __fitosf
#include "runtime_symbols.hpp"
#include "runtime_machine.hpp"
#include "float_helpers.hpp"

// ---------------------------------------------------------------------------
// ___uint2fs — unsigned 16-bit int to float
// Result in HL:DE (HL=high word, DE=low word)
// ---------------------------------------------------------------------------

TEST(uint2fs_zero)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(uint2fs_one)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 1));
    REQUIRE(feq(g_rt->result_float_hlde(), 1.0f));
}

TEST(uint2fs_hundred)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 100));
    REQUIRE(feq(g_rt->result_float_hlde(), 100.0f));
}

TEST(uint2fs_max_uint8)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 255));
    REQUIRE(feq(g_rt->result_float_hlde(), 255.0f));
}

TEST(uint2fs_1000)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 1000));
    REQUIRE(feq(g_rt->result_float_hlde(), 1000.0f));
}

TEST(uint2fs_32768)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 32768));
    REQUIRE(feq(g_rt->result_float_hlde(), 32768.0f));
}

TEST(uint2fs_max_uint16)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::uint2fs, 65535));
    REQUIRE(feq(g_rt->result_float_hlde(), 65535.0f));
}

// ---------------------------------------------------------------------------
// ___sint2fs — signed 16-bit int to float
// ---------------------------------------------------------------------------

TEST(sint2fs_zero)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(sint2fs_one)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, 1));
    REQUIRE(feq(g_rt->result_float_hlde(), 1.0f));
}

TEST(sint2fs_minus_one)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, (uint16_t)(-1)));
    REQUIRE(feq(g_rt->result_float_hlde(), -1.0f));
}

TEST(sint2fs_minus_100)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, (uint16_t)(-100)));
    REQUIRE(feq(g_rt->result_float_hlde(), -100.0f));
}

TEST(sint2fs_max_int16)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, 32767));
    REQUIRE(feq(g_rt->result_float_hlde(), 32767.0f));
}

TEST(sint2fs_min_int16)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::sint2fs, (uint16_t)(-32768)));
    REQUIRE(feq(g_rt->result_float_hlde(), -32768.0f));
}

// ---------------------------------------------------------------------------
// ___uchar2fs — unsigned char to float (A=value)
// ---------------------------------------------------------------------------

TEST(uchar2fs_zero)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::uchar2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(uchar2fs_100)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::uchar2fs, 100));
    REQUIRE(feq(g_rt->result_float_hlde(), 100.0f));
}

TEST(uchar2fs_255)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::uchar2fs, 255));
    REQUIRE(feq(g_rt->result_float_hlde(), 255.0f));
}

// ---------------------------------------------------------------------------
// ___schar2fs — signed char to float (A=value)
// ---------------------------------------------------------------------------

TEST(schar2fs_zero)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::schar2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(schar2fs_positive)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::schar2fs, 127));
    REQUIRE(feq(g_rt->result_float_hlde(), 127.0f));
}

TEST(schar2fs_negative)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::schar2fs, (uint8_t)(-1)));
    REQUIRE(feq(g_rt->result_float_hlde(), -1.0f));
}

TEST(schar2fs_min)
{
    REQUIRE(g_rt->call_char_to_float(rt_sym::schar2fs, (uint8_t)(-128)));
    REQUIRE(feq(g_rt->result_float_hlde(), -128.0f));
}

// ---------------------------------------------------------------------------
// ___ulong2fs — unsigned 32-bit to float
// ABI: HL=low word, DE=high word (NOTE: opposite of normal 32-bit ABI)
// Result: HL:DE (HL=high, DE=low)
// ---------------------------------------------------------------------------

TEST(ulong2fs_zero)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(ulong2fs_one)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, 1));
    REQUIRE(feq(g_rt->result_float_hlde(), 1.0f));
}

TEST(ulong2fs_million)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, 1000000));
    REQUIRE(feq(g_rt->result_float_hlde(), 1000000.0f));
}

TEST(ulong2fs_max_uint16)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, 65535));
    REQUIRE(feq(g_rt->result_float_hlde(), 65535.0f));
}

TEST(ulong2fs_large)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::ulong2fs, 0x7FFFFFFF));
    REQUIRE(feq(g_rt->result_float_hlde(), (float)0x7FFFFFFFu, 1e-5f));
}

// ---------------------------------------------------------------------------
// ___slong2fs — signed 32-bit to float (HL:DE = signed long, HL=high, DE=low)
// ---------------------------------------------------------------------------

TEST(slong2fs_zero)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::slong2fs, 0));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}

TEST(slong2fs_positive)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::slong2fs, 100000));
    REQUIRE(feq(g_rt->result_float_hlde(), 100000.0f));
}

TEST(slong2fs_negative)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::slong2fs, (uint32_t)(-100000)));
    REQUIRE(feq(g_rt->result_float_hlde(), -100000.0f));
}

TEST(slong2fs_min_int32)
{
    REQUIRE(g_rt->call_long_to_float(rt_sym::slong2fs, (uint32_t)0x80000000));
    REQUIRE(feq(g_rt->result_float_hlde(), -2147483648.0f, 1e-4f));
}

// ---------------------------------------------------------------------------
// ___fs2sint — float to signed int16 (truncate toward zero)
// Input: float in HL:DE. Result: DE = signed 16-bit int.
// ---------------------------------------------------------------------------

TEST(fs2sint_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 1.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)1);
}

TEST(fs2sint_neg_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, -1.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)(-1));
}

TEST(fs2sint_truncate_pos)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 3.9f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)3);
}

TEST(fs2sint_truncate_neg)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, -3.9f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)(-3));
}

TEST(fs2sint_zero)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 0.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)0);
}

TEST(fs2sint_clamp_max)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 40000.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)32767);
}

TEST(fs2sint_clamp_min)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, -40000.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)(-32768));
}

TEST(fs2sint_max_valid)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 32767.0f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)32767);
}

TEST(fs2sint_fractional_less_than_1)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2sint, 0.5f));
    REQUIRE_EQ((int16_t)g_rt->snap().de, (int16_t)0);
}

// ---------------------------------------------------------------------------
// ___fs2uint — float to unsigned int16 (truncate toward zero)
// Result: DE = unsigned 16-bit int.
// ---------------------------------------------------------------------------

TEST(fs2uint_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uint, 1.0f));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)1);
}

TEST(fs2uint_negative_clamps_to_zero)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uint, -5.0f));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)0);
}

TEST(fs2uint_truncate)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uint, 99.9f));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)99);
}

TEST(fs2uint_clamp_max)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uint, 100000.0f));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)65535);
}

TEST(fs2uint_zero)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uint, 0.0f));
    REQUIRE_EQ(g_rt->snap().de, (uint16_t)0);
}

// ---------------------------------------------------------------------------
// ___fs2schar — float to signed char; result in A register
// ---------------------------------------------------------------------------

TEST(fs2schar_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2schar, 1.0f));
    REQUIRE_EQ(g_rt->result_a_s8(), (int8_t)1);
}

TEST(fs2schar_neg)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2schar, -50.0f));
    REQUIRE_EQ(g_rt->result_a_s8(), (int8_t)(-50));
}

TEST(fs2schar_exact_min)
{
    // ___fs2schar truncates to low byte of int16 — no int8 saturation.
    // -128.0f → fs2sint → DE=0xFF80 → A=E=0x80 = (int8_t)-128
    REQUIRE(g_rt->call_float1(rt_sym::fs2schar, -128.0f));
    REQUIRE_EQ(g_rt->result_a_s8(), (int8_t)(-128));
}

TEST(fs2schar_exact_max)
{
    // 127.0f → fs2sint → DE=0x007F → A=E=0x7F = (int8_t)127
    REQUIRE(g_rt->call_float1(rt_sym::fs2schar, 127.0f));
    REQUIRE_EQ(g_rt->result_a_s8(), (int8_t)127);
}

// ---------------------------------------------------------------------------
// ___fs2uchar — float to unsigned char; result in A register
// ---------------------------------------------------------------------------

TEST(fs2uchar_basic)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uchar, 100.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)100);
}

TEST(fs2uchar_neg_clamps)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2uchar, -1.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)0);
}

TEST(fs2uchar_exact_max)
{
    // ___fs2uchar truncates to low byte of uint16 — no uint8 saturation.
    // 255.0f → fs2uint → DE=0x00FF → A=E=0xFF = 255
    REQUIRE(g_rt->call_float1(rt_sym::fs2uchar, 255.0f));
    REQUIRE_EQ(g_rt->result_a(), (uint8_t)255);
}

// ---------------------------------------------------------------------------
// ___fs2slong — float to signed int32 (truncate toward zero)
// Result: HL:DE (HL=high, DE=low) — use result32()
// ---------------------------------------------------------------------------

TEST(fs2slong_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, 1.0f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1);
}

TEST(fs2slong_neg)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, -1.0f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)(-1));
}

TEST(fs2slong_large)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, 1000000.0f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)1000000);
}

TEST(fs2slong_truncate_pos)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, 99.9f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)99);
}

TEST(fs2slong_clamp_max)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, 3.0e10f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0x7FFFFFFF);
}

TEST(fs2slong_clamp_min)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2slong, -3.0e10f));
    REQUIRE_EQ((int32_t)g_rt->result32(), (int32_t)0x80000000);
}

// ---------------------------------------------------------------------------
// ___fs2ulong — float to unsigned int32 (truncate toward zero)
// Result: HL:DE (HL=high, DE=low) — use result32()
// ---------------------------------------------------------------------------

TEST(fs2ulong_one)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2ulong, 1.0f));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)1);
}

TEST(fs2ulong_neg_clamps)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2ulong, -1.0f));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0);
}

TEST(fs2ulong_large)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2ulong, 4000000000.0f));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)4000000000u);
}

TEST(fs2ulong_clamp_max)
{
    REQUIRE(g_rt->call_float1(rt_sym::fs2ulong, 5.0e10f));
    REQUIRE_EQ(g_rt->result32(), (uint32_t)0xFFFFFFFF);
}

// ---------------------------------------------------------------------------
// __fstoi — stub: returns 0 regardless of input
// ---------------------------------------------------------------------------

TEST(fstoi_stub_returns_zero)
{
    REQUIRE(g_rt->call_float1(rt_sym::fstoi, 42.0f));
    auto s = g_rt->snap();
    REQUIRE_EQ(s.hl, (uint16_t)0);
    REQUIRE_EQ(s.de, (uint16_t)0);
}

// ---------------------------------------------------------------------------
// __fitosf — stub: returns 0.0f regardless of input
// ---------------------------------------------------------------------------

TEST(fitosf_stub_returns_zero)
{
    REQUIRE(g_rt->call_int_to_float(rt_sym::fitosf, 42));
    REQUIRE(feq(g_rt->result_float_hlde(), 0.0f));
}
