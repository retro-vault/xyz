//
// complex.h — C11 <complex.h> for the xcc Z80 compiler.
//
// float _Complex is stored as 8 bytes: bytes 0-3 = real (soft-float),
// bytes 4-7 = imaginary (soft-float).  double == float on this target.
//
// Arithmetic operators (+, -, *) are lowered by the compiler to
// component-wise soft-float stubs (__fsadd/__fssub/__fsmul/__fsdiv).
//
// Replace the __fssqrt and __fsatan2 stubs in lib/runtime/ with real
// IEEE 754 implementations to get accurate cabs() and carg().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#ifndef _COMPLEX_H
#define _COMPLEX_H

// _Complex is a built-in keyword; expose the standard aliases.
#define complex   _Complex

// The imaginary unit I: a float _Complex with re=0, im=1.
extern float _Complex __complex_I;
#define _Complex_I  __complex_I
#define I           _Complex_I

// CMPLXF(x, y): construct a float _Complex constant.
#define CMPLXF(x, y)  ((float _Complex)((x) + (y) * I))
#define CMPLX(x, y)   CMPLXF((x), (y))
#define CMPLXL(x, y)  CMPLXF((x), (y))

// ----- Component accessors -------------------------------------------
// These work by treating the 8-byte complex as an array of two floats.

extern float __creal(float _Complex z);
extern float __cimag(float _Complex z);
#define crealf(z)  __creal(z)
#define cimagf(z)  __cimag(z)
#define creal(z)   __creal(z)
#define cimag(z)   __cimag(z)
#define creall(z)  __creal(z)
#define cimagl(z)  __cimag(z)

// ----- Complex arithmetic functions ----------------------------------

// Complex conjugate: conj(a + bi) = a - bi.
extern float _Complex conjf(float _Complex z);
#define conj(z)   conjf(z)
#define conjl(z)  conjf(z)

// Modulus: cabs(a + bi) = sqrt(a*a + b*b).
extern float cabsf(float _Complex z);
#define cabs(z)   cabsf(z)
#define cabsl(z)  cabsf(z)

// Phase angle: carg(a + bi) = atan2(b, a).
extern float cargf(float _Complex z);
#define carg(z)   cargf(z)
#define cargl(z)  cargf(z)

#endif // _COMPLEX_H
