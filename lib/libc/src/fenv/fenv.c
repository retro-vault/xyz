/*
 * fenv.c
 *
 * Minimal floating-point environment support for the xcc Z80 libc.
 *
 * The current soft-float runtime exposes one process-wide environment with
 * sticky exception bits and a remembered rounding mode. Arithmetic helpers do
 * not update these bits automatically yet, but programs may manipulate them
 * explicitly through the standard interfaces below.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <fenv.h>

const fenv_t __fe_dfl_env = { 0U, FE_TONEAREST };

static fenv_t __fe_current_env = { 0U, FE_TONEAREST };

static int __fe_round_is_valid(int round)
{
    return round == FE_TONEAREST ||
           round == FE_DOWNWARD ||
           round == FE_UPWARD ||
           round == FE_TOWARDZERO;
}

int feclearexcept(int excepts)
{
    __fe_current_env.excepts &= (fexcept_t)~(fexcept_t)excepts;
    return 0;
}

int fegetexceptflag(fexcept_t *restrict flagp, int excepts)
{
    if (flagp == 0) {
        return 1;
    }
    *flagp = __fe_current_env.excepts & (fexcept_t)excepts;
    return 0;
}

int feraiseexcept(int excepts)
{
    __fe_current_env.excepts |= ((fexcept_t)excepts & (fexcept_t)FE_ALL_EXCEPT);
    return 0;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
    fexcept_t mask;

    if (flagp == 0) {
        return 1;
    }

    mask = (fexcept_t)excepts & (fexcept_t)FE_ALL_EXCEPT;
    __fe_current_env.excepts &= (fexcept_t)~mask;
    __fe_current_env.excepts |= (*flagp & mask);
    return 0;
}

int fetestexcept(int excepts)
{
    return (int)(__fe_current_env.excepts & (fexcept_t)excepts);
}

int fegetround(void)
{
    return __fe_current_env.rounding;
}

int fesetround(int round)
{
    if (!__fe_round_is_valid(round)) {
        return 1;
    }
    __fe_current_env.rounding = round;
    return 0;
}

int fegetenv(fenv_t *envp)
{
    if (envp == 0) {
        return 1;
    }
    *envp = __fe_current_env;
    return 0;
}

int feholdexcept(fenv_t *envp)
{
    if (envp == 0) {
        return 1;
    }
    *envp = __fe_current_env;
    __fe_current_env.excepts = 0U;
    return 0;
}

int fesetenv(const fenv_t *envp)
{
    if (envp == 0) {
        return 1;
    }
    if (!__fe_round_is_valid(envp->rounding)) {
        return 1;
    }
    __fe_current_env = *envp;
    __fe_current_env.excepts &= (fexcept_t)FE_ALL_EXCEPT;
    return 0;
}

int feupdateenv(const fenv_t *envp)
{
    fexcept_t pending;

    if (envp == 0) {
        return 1;
    }

    pending = __fe_current_env.excepts;
    if (fesetenv(envp) != 0) {
        return 1;
    }
    __fe_current_env.excepts |= pending;
    return 0;
}
