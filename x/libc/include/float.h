/*
 * float.h
 *
 * Standard C23 floating-point characteristics for the xcc Z80 target.
 *
 * The C `float` model is selected by `--float-format=` and can be IEEE binary32,
 * IEEE binary16, or one of the fixed-point compatibility formats. `double`
 * remains the 64-bit software IEEE-754 runtime type, and `long double`
 * currently aliases `double`.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _FLOAT_H
#define _FLOAT_H

#define __STDC_VERSION_FLOAT_H__ 202311L

/* All xcc floating formats use a binary radix. */
#define FLT_RADIX 2

/* ------------------------------------------------------------------------- */
/* float characteristics                                                      */
/* ------------------------------------------------------------------------- */

#if defined(__XCC_FLOAT_FORMAT_IEEE16)

#define FLT_MANT_DIG       11
#define FLT_DIG            3
#define FLT_MIN_EXP        (-13)
#define FLT_MAX_EXP        16
#define FLT_MIN_10_EXP     (-4)
#define FLT_MAX_10_EXP     4
#define FLT_DECIMAL_DIG    5
#define FLT_MAX            6.5504000000000000e+04F
#define FLT_MIN            6.1035156250000000e-05F
#define FLT_TRUE_MIN       5.9604644775390625e-08F
#define FLT_EPSILON        9.7656250000000000e-04F
#define FLT_HAS_SUBNORM    1
#define FLT_IS_IEC_60559   1

#elif defined(__XCC_FLOAT_FORMAT_FIXED8_8)

#define FLT_MANT_DIG       9
#define FLT_DIG            2
#define FLT_MIN_EXP        (-7)
#define FLT_MAX_EXP        7
#define FLT_MIN_10_EXP     (-2)
#define FLT_MAX_10_EXP     2
#define FLT_DECIMAL_DIG    4
#define FLT_MAX            1.2798828125000000e+02F
#define FLT_MIN            3.9062500000000000e-03F
#define FLT_TRUE_MIN       FLT_MIN
#define FLT_EPSILON        3.9062500000000000e-03F
#define FLT_HAS_SUBNORM    0
#define FLT_IS_IEC_60559   0

#elif defined(__XCC_FLOAT_FORMAT_FIXED16_16)

#define FLT_MANT_DIG       17
#define FLT_DIG            4
#define FLT_MIN_EXP        (-15)
#define FLT_MAX_EXP        15
#define FLT_MIN_10_EXP     (-4)
#define FLT_MAX_10_EXP     4
#define FLT_DECIMAL_DIG    6
#define FLT_MAX            3.2767999954223633e+04F
#define FLT_MIN            1.5258789062500000e-05F
#define FLT_TRUE_MIN       FLT_MIN
#define FLT_EPSILON        1.5258789062500000e-05F
#define FLT_HAS_SUBNORM    0
#define FLT_IS_IEC_60559   0

#elif defined(__XCC_FLOAT_FORMAT_FIXED24_8)

#define FLT_MANT_DIG       9
#define FLT_DIG            2
#define FLT_MIN_EXP        (-7)
#define FLT_MAX_EXP        23
#define FLT_MIN_10_EXP     (-2)
#define FLT_MAX_10_EXP     6
#define FLT_DECIMAL_DIG    8
#define FLT_MAX            8.3886079882812500e+06F
#define FLT_MIN            3.9062500000000000e-03F
#define FLT_TRUE_MIN       FLT_MIN
#define FLT_EPSILON        3.9062500000000000e-03F
#define FLT_HAS_SUBNORM    0
#define FLT_IS_IEC_60559   0

#else

#define FLT_MANT_DIG       24
#define FLT_DIG            6
#define FLT_MIN_EXP        (-125)
#define FLT_MAX_EXP        128
#define FLT_MIN_10_EXP     (-37)
#define FLT_MAX_10_EXP     38
#define FLT_DECIMAL_DIG    9
#define FLT_MAX            3.4028234663852886e+38F
#define FLT_MIN            1.1754943508222875e-38F
#define FLT_TRUE_MIN       1.4012984643248171e-45F
#define FLT_EPSILON        1.1920928955078125e-07F
#define FLT_HAS_SUBNORM    1
#define FLT_IS_IEC_60559   1

#endif

/* ------------------------------------------------------------------------- */
/* double characteristics (64-bit software IEEE-754 binary64)               */
/* ------------------------------------------------------------------------- */

#define DBL_MANT_DIG       53
#define DBL_DIG            15
#define DBL_MIN_EXP        (-1021)
#define DBL_MAX_EXP        1024
#define DBL_MIN_10_EXP     (-307)
#define DBL_MAX_10_EXP     308
#define DBL_DECIMAL_DIG    17
#define DBL_MAX            1.7976931348623157e+308
#define DBL_MIN            2.2250738585072014e-308
#define DBL_TRUE_MIN       4.9406564584124654e-324
#define DBL_EPSILON        2.2204460492503131e-16
#define DBL_HAS_SUBNORM    1
#define DBL_IS_IEC_60559   1

/* long double currently aliases the double runtime. */
#define LDBL_MANT_DIG      DBL_MANT_DIG
#define LDBL_DIG           DBL_DIG
#define LDBL_MIN_EXP       DBL_MIN_EXP
#define LDBL_MAX_EXP       DBL_MAX_EXP
#define LDBL_MIN_10_EXP    DBL_MIN_10_EXP
#define LDBL_MAX_10_EXP    DBL_MAX_10_EXP
#define LDBL_DECIMAL_DIG   DBL_DECIMAL_DIG
#define LDBL_MAX           DBL_MAX
#define LDBL_MIN           DBL_MIN
#define LDBL_TRUE_MIN      DBL_TRUE_MIN
#define LDBL_EPSILON       DBL_EPSILON
#define LDBL_HAS_SUBNORM   DBL_HAS_SUBNORM
#define LDBL_IS_IEC_60559  DBL_IS_IEC_60559

/* All floating expressions are evaluated in their nominal precision. */
#define FLT_EVAL_METHOD 0

/* Decimal digits required to round-trip the widest floating type. */
#define DECIMAL_DIG 17

/* C23 signaling NaN convenience macros. */
#define FLT_SNAN __builtin_nansf("")
#define DBL_SNAN __builtin_nans("")
#define LDBL_SNAN __builtin_nansl("")

#endif /* _FLOAT_H */
