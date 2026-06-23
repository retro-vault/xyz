/*
 * float.h
 *
 * Standard C23 floating-point characteristics for the xcc Z80 target.
 *
 * xcc uses 4-byte IEEE 754 single-precision soft-float for both float and
 * double. long double is also mapped to the same software format on this
 * target.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _FLOAT_H
#define _FLOAT_H

#define __STDC_VERSION_FLOAT_H__ 202311L

/* Exponent radix. */
#define FLT_RADIX       2

/* float characteristics. */
#define FLT_MANT_DIG    24
#define FLT_DIG         6
#define FLT_MIN_EXP     (-125)
#define FLT_MAX_EXP     128
#define FLT_MIN_10_EXP  (-37)
#define FLT_MAX_10_EXP  38
#define FLT_MAX         3.40282347e+38F
#define FLT_MIN         1.17549435e-38F
#define FLT_EPSILON     1.19209290e-07F

/* double aliases float on this target. */
#define DBL_MANT_DIG    FLT_MANT_DIG
#define DBL_DIG         FLT_DIG
#define DBL_MIN_EXP     FLT_MIN_EXP
#define DBL_MAX_EXP     FLT_MAX_EXP
#define DBL_MIN_10_EXP  FLT_MIN_10_EXP
#define DBL_MAX_10_EXP  FLT_MAX_10_EXP
#define DBL_MAX         FLT_MAX
#define DBL_MIN         FLT_MIN
#define DBL_EPSILON     FLT_EPSILON

/* long double also aliases float on this target. */
#define LDBL_MANT_DIG   FLT_MANT_DIG
#define LDBL_DIG        FLT_DIG
#define LDBL_MIN_EXP    FLT_MIN_EXP
#define LDBL_MAX_EXP    FLT_MAX_EXP
#define LDBL_MIN_10_EXP FLT_MIN_10_EXP
#define LDBL_MAX_10_EXP FLT_MAX_10_EXP
#define LDBL_MAX        FLT_MAX
#define LDBL_MIN        FLT_MIN
#define LDBL_EPSILON    FLT_EPSILON

/* All floating expressions are evaluated in float precision. */
#define FLT_EVAL_METHOD 0

/* Decimal digits required to round-trip the widest floating type. */
#define DECIMAL_DIG     9

/* C23 signaling NaN convenience macros. */
#define FLT_SNAN        __builtin_nansf("")
#define DBL_SNAN        __builtin_nans("")
#define LDBL_SNAN       __builtin_nansl("")

#endif /* _FLOAT_H */
