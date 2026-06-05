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

#endif /* _TGMATH_H */
