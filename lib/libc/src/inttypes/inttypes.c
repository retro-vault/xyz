/*
 * inttypes.c
 *
 * Max-width integer helpers for the xcc Z80 libc.
 *
 * These wrappers bridge the standard inttypes entry points onto the
 * corresponding stdlib helpers implemented for this freestanding target.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <inttypes.h>
#include <stdlib.h>

intmax_t imaxabs(intmax_t j)
{
    return llabs(j);
}

imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom)
{
    lldiv_t raw;
    imaxdiv_t result;

    raw = lldiv(numer, denom);
    result.quot = raw.quot;
    result.rem = raw.rem;
    return result;
}

intmax_t strtoimax(const char *restrict nptr, char **restrict endptr, int base)
{
    return strtoll(nptr, endptr, base);
}

uintmax_t strtoumax(const char *restrict nptr, char **restrict endptr, int base)
{
    return strtoull(nptr, endptr, base);
}
