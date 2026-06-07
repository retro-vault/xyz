/*
 * math.h
 *
 * Standard C23 mathematics support for the xcc Z80 target.
 *
 * float, double, and long double are all 32-bit IEEE-754 single precision on
 * this target, so each routine exposes the f-suffixed, unsuffixed, and
 * l-suffixed names from a single implementation.
 *
 * This header declares the full standard interface.  The classification,
 * sign/abs, square root, atan2, nearest-integer, decomposition, scaling and
 * min/max families are implemented (mostly in hand-written assembly).  The
 * transcendental family is declared for source compatibility but not all
 * entry points are linkable yet; see docs/todo/STDLIB.md for status.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _MATH_H
#define _MATH_H

#define __STDC_VERSION_MATH_H__ 202311L

#include <float.h>

/* ------------------------------------------------------------------------- */
/* Types                                                                     */
/* ------------------------------------------------------------------------- */
typedef float  float_t;
typedef double double_t;

/* ------------------------------------------------------------------------- */
/* Classification result codes and special values                           */
/* ------------------------------------------------------------------------- */
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define FP_ILOGB0   (-32768)        /* INT_MIN: ilogb(0)   */
#define FP_ILOGBNAN (-32768)        /* INT_MIN: ilogb(NaN) */

#define HUGE_VALF __builtin_inff()
#define HUGE_VAL  __builtin_inf()
#define HUGE_VALL __builtin_infl()
#define INFINITY  __builtin_inff()
#define NAN       __builtin_nanf("")

/* errno-style error reporting is not performed by this soft-float library. */
#define MATH_ERRNO     1
#define MATH_ERREXCEPT 2
#define math_errhandling 0

/* ------------------------------------------------------------------------- */
/* Common mathematical constants (POSIX)                                     */
/* ------------------------------------------------------------------------- */
#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.43429448190325182765
#define M_LN2      0.69314718055994530942
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.78539816339744830962
#define M_1_PI     0.31830988618379067154
#define M_2_PI     0.63661977236758134308
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.70710678118654752440

/* ------------------------------------------------------------------------- */
/* Classification and comparison macros                                      */
/* ------------------------------------------------------------------------- */
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

#define isgreater(x, y)      (!isunordered((x), (y)) && (x) >  (y))
#define isgreaterequal(x, y) (!isunordered((x), (y)) && (x) >= (y))
#define isless(x, y)         (!isunordered((x), (y)) && (x) <  (y))
#define islessequal(x, y)    (!isunordered((x), (y)) && (x) <= (y))
#define islessgreater(x, y)  (!isunordered((x), (y)) && ((x) < (y) || (x) > (y)))
#define isunordered(x, y)    (isnan((x)) || isnan((y)))

/* ------------------------------------------------------------------------- */
/* Trigonometric, hyperbolic, exponential and logarithmic                    */
/* (declared for source compatibility; see STDLIB.md for link status)        */
/* ------------------------------------------------------------------------- */
float       sinf(float x);   double      sin(double x);   long double sinl(long double x);
float       cosf(float x);   double      cos(double x);   long double cosl(long double x);
float       tanf(float x);   double      tan(double x);   long double tanl(long double x);
float       asinf(float x);  double      asin(double x);  long double asinl(long double x);
float       acosf(float x);  double      acos(double x);  long double acosl(long double x);
float       atanf(float x);  double      atan(double x);  long double atanl(long double x);
float       atan2f(float y, float x);
double      atan2(double y, double x);
long double atan2l(long double y, long double x);
float       sinhf(float x);  double      sinh(double x);  long double sinhl(long double x);
float       coshf(float x);  double      cosh(double x);  long double coshl(long double x);
float       tanhf(float x);  double      tanh(double x);  long double tanhl(long double x);
float       asinhf(float x); double      asinh(double x); long double asinhl(long double x);
float       acoshf(float x); double      acosh(double x); long double acoshl(long double x);
float       atanhf(float x); double      atanh(double x); long double atanhl(long double x);

float       expf(float x);   double      exp(double x);   long double expl(long double x);
float       exp2f(float x);  double      exp2(double x);  long double exp2l(long double x);
float       expm1f(float x); double      expm1(double x); long double expm1l(long double x);
float       logf(float x);   double      log(double x);   long double logl(long double x);
float       log2f(float x);  double      log2(double x);  long double log2l(long double x);
float       log10f(float x); double      log10(double x); long double log10l(long double x);
float       log1pf(float x); double      log1p(double x); long double log1pl(long double x);

/* ------------------------------------------------------------------------- */
/* Power and absolute value                                                  */
/* ------------------------------------------------------------------------- */
float       powf(float x, float y);
double      pow(double x, double y);
long double powl(long double x, long double y);
float       sqrtf(float x);  double      sqrt(double x);  long double sqrtl(long double x);
float       cbrtf(float x);  double      cbrt(double x);  long double cbrtl(long double x);
float       hypotf(float x, float y);
double      hypot(double x, double y);
long double hypotl(long double x, long double y);
float       fabsf(float x);  double      fabs(double x);  long double fabsl(long double x);

/* ------------------------------------------------------------------------- */
/* Error and gamma                                                           */
/* ------------------------------------------------------------------------- */
float       erff(float x);    double      erf(double x);    long double erfl(long double x);
float       erfcf(float x);   double      erfc(double x);   long double erfcl(long double x);
float       tgammaf(float x); double      tgamma(double x); long double tgammal(long double x);
float       lgammaf(float x); double      lgamma(double x); long double lgammal(long double x);

/* ------------------------------------------------------------------------- */
/* Nearest-integer  (implemented)                                            */
/* ------------------------------------------------------------------------- */
float       ceilf(float x);   double      ceil(double x);   long double ceill(long double x);
float       floorf(float x);  double      floor(double x);  long double floorl(long double x);
float       truncf(float x);  double      trunc(double x);  long double truncl(long double x);
float       roundf(float x);  double      round(double x);  long double roundl(long double x);
long        lroundf(float x); long        lround(double x); long        lroundl(long double x);
long long   llroundf(float x);long long   llround(double x);long long   llroundl(long double x);
float       rintf(float x);   double      rint(double x);   long double rintl(long double x);
long        lrintf(float x);  long        lrint(double x);  long        lrintl(long double x);
long long   llrintf(float x); long long   llrint(double x); long long   llrintl(long double x);
float       nearbyintf(float x); double   nearbyint(double x); long double nearbyintl(long double x);

/* ------------------------------------------------------------------------- */
/* Decomposition, scaling and manipulation  (implemented)                    */
/* ------------------------------------------------------------------------- */
float       frexpf(float x, int *exp);
double      frexp(double x, int *exp);
long double frexpl(long double x, int *exp);
float       ldexpf(float x, int exp);
double      ldexp(double x, int exp);
long double ldexpl(long double x, int exp);
float       modff(float x, float *iptr);
double      modf(double x, double *iptr);
long double modfl(long double x, long double *iptr);
float       scalbnf(float x, int n);
double      scalbn(double x, int n);
long double scalbnl(long double x, int n);
float       scalblnf(float x, long n);
double      scalbln(double x, long n);
long double scalblnl(long double x, long n);
int         ilogbf(float x); int    ilogb(double x); int    ilogbl(long double x);
float       logbf(float x);  double logb(double x);  long double logbl(long double x);
float       significandf(float x);
double      significand(double x);
float       copysignf(float mag, float sign);
double      copysign(double mag, double sign);
long double copysignl(long double mag, long double sign);
float       nextafterf(float x, float y);
double      nextafter(double x, double y);
long double nextafterl(long double x, long double y);
float       nanf(const char *tag);
double      nan(const char *tag);
long double nanl(const char *tag);

/* ------------------------------------------------------------------------- */
/* Remainder and FMA                                                         */
/* ------------------------------------------------------------------------- */
float       fmodf(float x, float y);
double      fmod(double x, double y);
long double fmodl(long double x, long double y);
float       remainderf(float x, float y);
double      remainder(double x, double y);
long double remainderl(long double x, long double y);
float       remquof(float x, float y, int *quo);
double      remquo(double x, double y, int *quo);
long double remquol(long double x, long double y, int *quo);
float       fmaf(float x, float y, float z);
double      fma(double x, double y, double z);
long double fmal(long double x, long double y, long double z);

/* ------------------------------------------------------------------------- */
/* Minimum, maximum and positive difference  (implemented)                   */
/* ------------------------------------------------------------------------- */
float       fmaxf(float x, float y);
double      fmax(double x, double y);
long double fmaxl(long double x, long double y);
float       fminf(float x, float y);
double      fmin(double x, double y);
long double fminl(long double x, long double y);
float       fdimf(float x, float y);
double      fdim(double x, double y);
long double fdiml(long double x, long double y);

#endif /* _MATH_H */
