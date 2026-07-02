/*
 * fenv.h
 *
 * Standard C23 floating-point environment support for the xcc Z80 target.
 *
 * The current soft-float runtime models one process-wide floating-point
 * environment with sticky exception flags and a remembered rounding-mode
 * selector. Arithmetic helpers do not yet raise exceptions automatically, but
 * programs can still inspect, clear, and raise the standard flags explicitly.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _FENV_H
#define _FENV_H

#define __STDC_VERSION_FENV_H__ 202311L

typedef unsigned int fexcept_t;

typedef struct fenv_t {
    fexcept_t excepts;
    int       rounding;
} fenv_t;

#define FE_INVALID    0x01
#define FE_DIVBYZERO  0x02
#define FE_OVERFLOW   0x04
#define FE_UNDERFLOW  0x08
#define FE_INEXACT    0x10
#define FE_ALL_EXCEPT (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT)

#define FE_TONEAREST  0
#define FE_DOWNWARD   1
#define FE_UPWARD     2
#define FE_TOWARDZERO 3

extern const fenv_t __fe_dfl_env;
#define FE_DFL_ENV (&__fe_dfl_env)

int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t *restrict flagp, int excepts);
int feraiseexcept(int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int fetestexcept(int excepts);

int fegetround(void);
int fesetround(int round);

int fegetenv(fenv_t *envp);
int feholdexcept(fenv_t *envp);
int fesetenv(const fenv_t *envp);
int feupdateenv(const fenv_t *envp);

#endif /* _FENV_H */
