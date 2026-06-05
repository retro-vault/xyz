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
extern float _Complex __complex_I;
#define _Complex_I  __complex_I
#define I           _Complex_I

/* Construct float-, double-, or long-double complex values. */
#define CMPLXF(x, y)  ((float _Complex)((x) + (y) * I))
#define CMPLX(x, y)   CMPLXF((x), (y))
#define CMPLXL(x, y)  CMPLXF((x), (y))

/* Component accessors. */
extern float __creal(float _Complex z);
extern float __cimag(float _Complex z);
#define crealf(z)  __creal(z)
#define cimagf(z)  __cimag(z)
#define creal(z)   __creal(z)
#define cimag(z)   __cimag(z)
#define creall(z)  __creal(z)
#define cimagl(z)  __cimag(z)

/* Complex conjugation. */
extern float _Complex conjf(float _Complex z);
#define conj(z)   conjf(z)
#define conjl(z)  conjf(z)

/* Complex magnitude. */
extern float cabsf(float _Complex z);
#define cabs(z)   cabsf(z)
#define cabsl(z)  cabsf(z)

/* Complex phase angle. */
extern float cargf(float _Complex z);
#define carg(z)   cargf(z)
#define cargl(z)  cargf(z)

#endif /* _COMPLEX_H */
