//
// Small assertion helpers shared by executable xcc regression tests.
// Tests are self-checking: returning 0 means pass, and returning a
// small explicit failure code means fail.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#ifndef XCC_EXEC_TEST_H
#define XCC_EXEC_TEST_H

union xcc_float_bits {
    unsigned long u;
    float         f;
};

#define XCC_FAIL(code) return (code)
#define XCC_CHECK_ID(code, cond) \
    do { if (!(cond)) XCC_FAIL(code); } while (0)
#define XCC_CHECK_EQ_INT_ID(code, actual, expected) \
    do { if ((actual) != (expected)) XCC_FAIL(code); } while (0)
#define XCC_CHECK_EQ_UINT_ID(code, actual, expected) \
    do { if ((unsigned int)(actual) != (unsigned int)(expected)) XCC_FAIL(code); } while (0)
#define XCC_CHECK_EQ_LONG_ID(code, actual, expected) \
    do { if ((long)(actual) != (long)(expected)) XCC_FAIL(code); } while (0)
#define XCC_CHECK_EQ_ULONG_ID(code, actual, expected) \
    do { if ((unsigned long)(actual) != (unsigned long)(expected)) XCC_FAIL(code); } while (0)
#define XCC_CHECK_EQ_U32_ID(code, actual, expected_lo, expected_hi) \
    do { \
        unsigned long xcc_u32_value_ = (actual); \
        unsigned int *xcc_u32_words_ = (unsigned int *)&xcc_u32_value_; \
        if (xcc_u32_words_[0] != (unsigned int)(expected_lo)) XCC_FAIL(code); \
        if (xcc_u32_words_[1] != (unsigned int)(expected_hi)) XCC_FAIL(code); \
    } while (0)

#endif
