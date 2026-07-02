/*
 * tgmath.h
 *
 * Type-generic math convenience macros for the currently implemented xcc Z80
 * math and complex subset.
 *
 * The current surface covers the runtime-backed real helpers and the complex
 * accessors already shipped in libc.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _TGMATH_H
#define _TGMATH_H

#include <complex.h>
#include <math.h>

#define fabs(value) _Generic((value), \
    float: fabsf, \
    double: fabs, \
    long double: fabsl \
)(value)

#define sqrt(value) _Generic((value), \
    float: sqrtf, \
    double: sqrt, \
    long double: sqrtl \
)(value)

#define atan2(y, x) _Generic(((y) + (x)), \
    float: atan2f, \
    double: atan2, \
    long double: atan2l \
)((y), (x))

#define creal(value) _Generic((value), \
    float _Complex: __creal, \
    double _Complex: __creal, \
    long double _Complex: __creal \
)(value)

#define cimag(value) _Generic((value), \
    float _Complex: __cimag, \
    double _Complex: __cimag, \
    long double _Complex: __cimag \
)(value)

#define conj(value) _Generic((value), \
    float _Complex: conjf, \
    double _Complex: conjf, \
    long double _Complex: conjf \
)(value)

#define cabs(value) _Generic((value), \
    float _Complex: cabsf, \
    double _Complex: cabsf, \
    long double _Complex: cabsf \
)(value)

#define carg(value) _Generic((value), \
    float _Complex: cargf, \
    double _Complex: cargf, \
    long double _Complex: cargf \
)(value)

#define cexp(value) _Generic((value), \
    float _Complex: cexpf, \
    double _Complex: cexpf, \
    long double _Complex: cexpf \
)(value)

#define clog(value) _Generic((value), \
    float _Complex: clogf, \
    double _Complex: clogf, \
    long double _Complex: clogf \
)(value)

#define cpow(x, y) _Generic(((x) + (y)), \
    float _Complex: cpowf, \
    double _Complex: cpowf, \
    long double _Complex: cpowf \
)((x), (y))

#define csqrt(value) _Generic((value), \
    float _Complex: csqrtf, \
    double _Complex: csqrtf, \
    long double _Complex: csqrtf \
)(value)

#define csin(value) _Generic((value), \
    float _Complex: csinf, \
    double _Complex: csinf, \
    long double _Complex: csinf \
)(value)

#define ccos(value) _Generic((value), \
    float _Complex: ccosf, \
    double _Complex: ccosf, \
    long double _Complex: ccosf \
)(value)

#define ctan(value) _Generic((value), \
    float _Complex: ctanf, \
    double _Complex: ctanf, \
    long double _Complex: ctanf \
)(value)

#define casin(value) _Generic((value), \
    float _Complex: casinf, \
    double _Complex: casinf, \
    long double _Complex: casinf \
)(value)

#define cacos(value) _Generic((value), \
    float _Complex: cacosf, \
    double _Complex: cacosf, \
    long double _Complex: cacosf \
)(value)

#define catan(value) _Generic((value), \
    float _Complex: catanf, \
    double _Complex: catanf, \
    long double _Complex: catanf \
)(value)

#define csinh(value) _Generic((value), \
    float _Complex: csinhf, \
    double _Complex: csinhf, \
    long double _Complex: csinhf \
)(value)

#define ccosh(value) _Generic((value), \
    float _Complex: ccoshf, \
    double _Complex: ccoshf, \
    long double _Complex: ccoshf \
)(value)

#define ctanh(value) _Generic((value), \
    float _Complex: ctanhf, \
    double _Complex: ctanhf, \
    long double _Complex: ctanhf \
)(value)

#define casinh(value) _Generic((value), \
    float _Complex: casinhf, \
    double _Complex: casinhf, \
    long double _Complex: casinhf \
)(value)

#define cacosh(value) _Generic((value), \
    float _Complex: cacoshf, \
    double _Complex: cacoshf, \
    long double _Complex: cacoshf \
)(value)

#define catanh(value) _Generic((value), \
    float _Complex: catanhf, \
    double _Complex: catanhf, \
    long double _Complex: catanhf \
)(value)

#define cproj(value) _Generic((value), \
    float _Complex: cprojf, \
    double _Complex: cprojf, \
    long double _Complex: cprojf \
)(value)

#endif /* _TGMATH_H */
