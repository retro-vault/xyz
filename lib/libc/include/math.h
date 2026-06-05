/*
 * math.h
 *
 * Standard C23 mathematics support for the xcc Z80 target.
 *
 * The current target ships a small, honest soft-float math subset backed by
 * the existing runtime helpers: classification, sign handling, absolute value,
 * square root, and atan2. Wider transcendental coverage will follow once the
 * software math runtime grows beyond its present helper set.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _MATH_H
#define _MATH_H

#define __STDC_VERSION_MATH_H__ 202311L

#include <float.h>

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define M_E          2.71828182845904523536
#define M_PI         3.14159265358979323846

#define HUGE_VALF    __builtin_inff()
#define HUGE_VAL     __builtin_inf()
#define HUGE_VALL    __builtin_infl()
#define INFINITY     __builtin_inff()
#define NAN          __builtin_nanf("")

#define math_errhandling 0

int __libc_fpclassifyf(float value);
int __libc_signbitf(float value);
int __libc_isfinitef(float value);
int __libc_isinff(float value);
int __libc_isnanf(float value);

#define fpclassify(value) __libc_fpclassifyf((float)(value))
#define signbit(value)    __libc_signbitf((float)(value))
#define isfinite(value)   __libc_isfinitef((float)(value))
#define isinf(value)      __libc_isinff((float)(value))
#define isnan(value)      __libc_isnanf((float)(value))
#define isnormal(value)   (fpclassify((value)) == FP_NORMAL)

float fabsf(float value);
double fabs(double value);
long double fabsl(long double value);

float copysignf(float mag, float sign);
double copysign(double mag, double sign);
long double copysignl(long double mag, long double sign);

float sqrtf(float value);
double sqrt(double value);
long double sqrtl(long double value);

float atan2f(float y, float x);
double atan2(double y, double x);
long double atan2l(long double y, long double x);

#endif /* _MATH_H */
