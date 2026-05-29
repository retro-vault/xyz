/*
 * float.h — C11/C23 floating-point characteristics for the xcc Z80 target.
 *
 * xcc uses 4-byte IEEE 754 single-precision soft-float for both float and
 * double.  The values below reflect single-precision characteristics.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef _FLOAT_H
#define _FLOAT_H

/* Radix of exponent representation */
#define FLT_RADIX       2

/* float (32-bit IEEE 754 single) */
#define FLT_MANT_DIG    24
#define FLT_DIG         6
#define FLT_MIN_EXP     (-125)
#define FLT_MAX_EXP     128
#define FLT_MIN_10_EXP  (-37)
#define FLT_MAX_10_EXP  38
#define FLT_MAX         3.40282347e+38F
#define FLT_MIN         1.17549435e-38F
#define FLT_EPSILON     1.19209290e-07F

/* double == float on Z80 (no hardware FPU) */
#define DBL_MANT_DIG    FLT_MANT_DIG
#define DBL_DIG         FLT_DIG
#define DBL_MIN_EXP     FLT_MIN_EXP
#define DBL_MAX_EXP     FLT_MAX_EXP
#define DBL_MIN_10_EXP  FLT_MIN_10_EXP
#define DBL_MAX_10_EXP  FLT_MAX_10_EXP
#define DBL_MAX         FLT_MAX
#define DBL_MIN         FLT_MIN
#define DBL_EPSILON     FLT_EPSILON

/* long double == float on Z80 */
#define LDBL_MANT_DIG   FLT_MANT_DIG
#define LDBL_DIG        FLT_DIG
#define LDBL_MIN_EXP    FLT_MIN_EXP
#define LDBL_MAX_EXP    FLT_MAX_EXP
#define LDBL_MIN_10_EXP FLT_MIN_10_EXP
#define LDBL_MAX_10_EXP FLT_MAX_10_EXP
#define LDBL_MAX        FLT_MAX
#define LDBL_MIN        FLT_MIN
#define LDBL_EPSILON    FLT_EPSILON

/* Evaluation format: all in float precision */
#define FLT_EVAL_METHOD 0

/* Decimal digits of precision */
#define DECIMAL_DIG     9

/* C23: signaling NaN macros (soft-float stubs) */
#define FLT_SNAN        __builtin_nansf("")
#define DBL_SNAN        __builtin_nans("")
#define LDBL_SNAN       __builtin_nansl("")

#endif /* _FLOAT_H */
