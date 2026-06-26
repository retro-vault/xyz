/*
 * Z80 C23 Comprehensive Integer Test - ALL Sizes
 * Covers: 8-bit (char/int8_t), 16-bit (int/short), 32-bit (long/int32_t),
 *         64-bit (long long/int64_t), signed + unsigned for all.
 *
 * Tests: arithmetic, bitwise, shifts, promotions, arrays, pointers,
 *        structs, constants, overflow/wraparound, comparisons, etc.
 */

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static int tests_passed = 0;
static int tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s\n", msg); \
    } \
} while(0)

int main(void) {
    printf("=== Z80 C23 INTEGER TEST - ALL SIZES (8/16/32/64-bit) ===\n\n");

    printf("sizeof(char)=%zu, sizeof(short)=%zu, sizeof(int)=%zu, sizeof(long)=%zu, sizeof(long long)=%zu\n",
           sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long));
    printf("CHAR_BIT=%d, sizeof(int8_t)=%zu, sizeof(int64_t)=%zu\n\n",
           CHAR_BIT, sizeof(int8_t), sizeof(int64_t));

    /* ===================== 8-BIT ===================== */
    printf("--- 8-bit integers (char / int8_t / uint8_t) ---\n");

    int8_t i8 = 127;
    int8_t i8_neg = -128;
    uint8_t u8 = 255;

    TEST_ASSERT(i8 == 127, "int8_t max positive");
    TEST_ASSERT(i8_neg == -128, "int8_t min negative");
    TEST_ASSERT(u8 == 255, "uint8_t max");

    /* 8-bit arithmetic */
    TEST_ASSERT((int8_t)(i8 + 1) == -128, "int8_t overflow wrap (signed)");
    TEST_ASSERT((uint8_t)(u8 + 1) == 0, "uint8_t wraparound");
    TEST_ASSERT(i8 - i8_neg == 255, "int8_t subtraction");
    TEST_ASSERT((int8_t)(i8 * 2) == -2, "int8_t multiplication overflow");
    TEST_ASSERT((u8 / 2) == 127, "uint8_t division");

    /* 8-bit bitwise */
    TEST_ASSERT((i8 & 0x7F) == 127, "int8_t bitwise AND");
    TEST_ASSERT((u8 | 0x00) == 255, "uint8_t bitwise OR");
    TEST_ASSERT((~u8 & 0xFF) == 0, "uint8_t bitwise NOT");
    TEST_ASSERT((i8 ^ 0xFF) == -128, "int8_t XOR");

    /* 8-bit shifts */
    TEST_ASSERT((uint8_t)(u8 << 1) == 254, "uint8_t left shift");
    TEST_ASSERT((u8 >> 4) == 15, "uint8_t right shift");

    /* ===================== 16-BIT ===================== */
    printf("\n--- 16-bit integers (short / int16_t / uint16_t) ---\n");

    int16_t i16 = 32767;
    uint16_t u16 = 65535;

    TEST_ASSERT(i16 == 32767, "int16_t max");
    TEST_ASSERT(u16 == 65535, "uint16_t max");
    TEST_ASSERT((i16 + 1) == -32768, "int16_t overflow");
    TEST_ASSERT((u16 + 1) == 0, "uint16_t wrap");

    /* ===================== 32-BIT ===================== */
    printf("\n--- 32-bit integers (int / long / int32_t) ---\n");

    int32_t i32 = 2147483647L;
    uint32_t u32 = 0xFFFFFFFFu;

    TEST_ASSERT(i32 == 2147483647, "int32_t / long max");
    TEST_ASSERT(u32 == 0xFFFFFFFFu, "uint32_t max");
    TEST_ASSERT((i32 + 1) < 0, "int32_t signed overflow (wrap to negative)");
    TEST_ASSERT((u32 + 1u) == 0u, "uint32_t wraparound");

    /* 32-bit bitwise & shifts */
    TEST_ASSERT((i32 & 0x7FFFFFFF) == 2147483647, "int32_t bitwise");
    TEST_ASSERT((u32 >> 16) == 0xFFFF, "uint32_t right shift 16");

    /* ===================== 64-BIT ===================== */
    printf("\n--- 64-bit integers (long long / int64_t) ---\n");

#ifdef INT64_MAX
    int64_t i64 = 9223372036854775807LL;
    uint64_t u64 = 0xFFFFFFFFFFFFFFFFULL;

    TEST_ASSERT(i64 > 0, "int64_t max positive");
    TEST_ASSERT((uint64_t)(i64 + 1) == 0x8000000000000000ULL, "int64_t overflow to high bit");
    TEST_ASSERT((u64 + 1ULL) == 0ULL, "uint64_t full wraparound");

    /* 64-bit bitwise */
    TEST_ASSERT((i64 & 0xFFFFFFFFFFFFFFFFULL) == i64, "int64_t bitwise mask");
    TEST_ASSERT((u64 >> 32) == 0xFFFFFFFFULL, "uint64_t right shift 32");
    TEST_ASSERT((u64 << 1) == 0xFFFFFFFFFFFFFFFEULL, "uint64_t left shift");
#endif

    /* ===================== TYPE PROMOTIONS ===================== */
    printf("\n--- Type promotions & mixed operations ---\n");

    char c = 100;
    int i = 1000;
    long l = 100000L;

    TEST_ASSERT((c + i) == 1100, "char + int promotion");
    TEST_ASSERT((i + l) == 101000L, "int + long promotion");
    TEST_ASSERT((uint8_t)200 + (int8_t)(-50) == 150, "unsigned char + signed char");

    /* ===================== ARRAYS & POINTERS ===================== */
    printf("\n--- Arrays, pointers, structs ---\n");

    int8_t arr8[4] = {1, 2, 3, 4};
    uint32_t arr32[3] = {0x11111111, 0x22222222, 0x33333333};
    int64_t arr64[2] = {0x123456789ABCDEF0LL, -1LL};

    TEST_ASSERT(arr8[2] == 3, "int8_t array access");
    TEST_ASSERT(arr32[1] == 0x22222222, "uint32_t array");
    TEST_ASSERT(arr64[0] > 0, "int64_t array");

    int8_t *p8 = arr8;
    TEST_ASSERT(*(p8 + 1) == 2, "int8_t pointer arithmetic");

    struct MixedInts {
        int8_t   i8;
        uint16_t u16;
        int32_t  i32;
        int64_t  i64;
    };
    struct MixedInts s = { -5, 40000, 123456789, 987654321012345LL };
    TEST_ASSERT(s.i8 == -5 && s.u16 == 40000 && s.i32 > 0 && s.i64 > 0, "struct with all int sizes");

    /* ===================== LOOPS & CONTROL ===================== */
    printf("\n--- Loops and control flow ---\n");

    int sum = 0;
    for (int k = 0; k < 100; k++) sum += k;
    TEST_ASSERT(sum == 4950, "for loop sum 0..99");

    uint8_t u8sum = 0;
    for (uint8_t k = 0; k < 255; k++) u8sum += 1;
    TEST_ASSERT(u8sum == 255, "uint8_t loop to 255");

    /* ===================== CONSTANTS FROM LIMITS.H ===================== */
    printf("\n--- Limits constants ---\n");

    TEST_ASSERT(CHAR_MIN <= -128, "CHAR_MIN");
    TEST_ASSERT(UCHAR_MAX == 255, "UCHAR_MAX");
    TEST_ASSERT(INT_MAX > 30000, "INT_MAX reasonable");
    TEST_ASSERT(LONG_MAX > 1000000L, "LONG_MAX");
#ifdef LLONG_MAX
    TEST_ASSERT(LLONG_MAX > 1000000000000LL, "LLONG_MAX");
#endif

    printf("\n=== SUMMARY ===\n");
    printf("Integer tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("ALL INTEGER SIZES TEST PASSED SUCCESSFULLY!\n");
    } else {
        printf("Some tests failed — check integer codegen in your compiler.\n");
    }

    return 0;
}
