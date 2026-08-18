/*
 * complex.h
 *
 * Standard C23 complex-number support for the xcc Z80 target.
 *
 * The current target stores float _Complex values as two adjacent soft-float
 * components in the target's 8-byte complex layout. The compiler lowers
 * complex arithmetic operators directly, while libc supplies the standard
 * accessors plus small freestanding helpers such as conj, cabs, and carg.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _COMPLEX_H
#define _COMPLEX_H

#define __STDC_VERSION_COMPLEX_H__ 202311L

/* _Complex is a built-in keyword; expose the standard aliases. */
#define complex   _Complex

/* The imaginary unit constant for this target's float-complex layout. */
extern float _Complex _complex_I;
[[sdcc::sdccall(1)]] float _Complex _cmplxf(float real, float imag);
#define _Complex_I  _complex_I
#define I           _Complex_I

/* Construct float-, double-, or long-double complex values. */
#define CMPLXF(x, y)  _cmplxf((float)(x), (float)(y))
#define CMPLX(x, y)   CMPLXF((x), (y))
#define CMPLXL(x, y)  CMPLXF((x), (y))

/* Component accessors. */
[[sdcc::sdccall(1)]] float _creal(float _Complex z);
[[sdcc::sdccall(1)]] float _cimag(float _Complex z);
#define crealf(z)  _creal(z)
#define cimagf(z)  _cimag(z)
#define creal(z)   _creal(z)
#define cimag(z)   _cimag(z)
#define creall(z)  _creal(z)
#define cimagl(z)  _cimag(z)

/* Complex conjugation. */
[[sdcc::sdccall(1)]] float _Complex conjf(float _Complex z);
#define conj(z)   conjf(z)
#define conjl(z)  conjf(z)

/* Complex magnitude. */
[[sdcc::sdccall(1)]] float cabsf(float _Complex z);
#define cabs(z)   cabsf(z)
#define cabsl(z)  cabsf(z)

/* Complex phase angle. */
[[sdcc::sdccall(1)]] float cargf(float _Complex z);
#define carg(z)   cargf(z)
#define cargl(z)  cargf(z)

/* Complex exponential, logarithm, and square root. */
[[sdcc::sdccall(1)]] float _Complex cexpf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex clogf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex cpowf(float _Complex x, float _Complex y);
[[sdcc::sdccall(1)]] float _Complex csqrtf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex csinf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex ccosf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex ctanf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex casinf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex cacosf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex catanf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex csinhf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex ccoshf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex ctanhf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex casinhf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex cacoshf(float _Complex z);
[[sdcc::sdccall(1)]] float _Complex catanhf(float _Complex z);
#define cexp(z)   cexpf(z)
#define cexpl(z)  cexpf(z)
#define clog(z)   clogf(z)
#define clogl(z)  clogf(z)
#define cpow(x,y)  cpowf((x), (y))
#define cpowl(x,y) cpowf((x), (y))
#define csqrt(z)  csqrtf(z)
#define csqrtl(z) csqrtf(z)
#define csin(z)   csinf(z)
#define csinl(z)  csinf(z)
#define ccos(z)   ccosf(z)
#define ccosl(z)  ccosf(z)
#define ctan(z)   ctanf(z)
#define ctanl(z)  ctanf(z)
#define casin(z)  casinf(z)
#define casinl(z) casinf(z)
#define cacos(z)  cacosf(z)
#define cacosl(z) cacosf(z)
#define catan(z)  catanf(z)
#define catanl(z) catanf(z)
#define csinh(z)  csinhf(z)
#define csinhl(z) csinhf(z)
#define ccosh(z)  ccoshf(z)
#define ccoshl(z) ccoshf(z)
#define ctanh(z)  ctanhf(z)
#define ctanhl(z) ctanhf(z)
#define casinh(z)  casinhf(z)
#define casinhl(z) casinhf(z)
#define cacosh(z)  cacoshf(z)
#define cacoshl(z) cacoshf(z)
#define catanh(z)  catanhf(z)
#define catanhl(z) catanhf(z)

/* Projection onto the Riemann sphere branch cut. */
[[sdcc::sdccall(1)]] float _Complex cprojf(float _Complex z);
#define cproj(z)  cprojf(z)
#define cprojl(z) cprojf(z)

#endif /* _COMPLEX_H */
