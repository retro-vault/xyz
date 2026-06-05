/*
 * uchar.c
 *
 * UTF-16 and UTF-32 conversion helpers for the xcc Z80 libc.
 *
 * The current target's multibyte execution encoding is a stateless one-byte
 * encoding, so these helpers reduce to direct byte/code-unit mapping for the
 * unsigned-byte range.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <errno.h>
#include <limits.h>
#include <uchar.h>

static void __uchar_reset_state(mbstate_t *ps)
{
    if (ps != 0) {
        ps->__unused = 0U;
    }
}

static size_t __uchar_import(unsigned long *out,
                             const char *restrict s,
                             size_t n,
                             mbstate_t *restrict ps)
{
    unsigned char byte;

    if (s == 0) {
        __uchar_reset_state(ps);
        return 0U;
    }
    if (n == 0U) {
        return (size_t)-2;
    }

    byte = (unsigned char)s[0];
    __uchar_reset_state(ps);
    *out = (unsigned long)byte;
    return byte == 0U ? 0U : 1U;
}

static size_t __uchar_export(char *restrict s,
                             unsigned long value,
                             mbstate_t *restrict ps)
{
    __uchar_reset_state(ps);
    if (s == 0) {
        return 1U;
    }
    if (value > UCHAR_MAX) {
        errno = EILSEQ;
        return (size_t)-1;
    }
    s[0] = (char)(unsigned char)value;
    return 1U;
}

size_t mbrtoc16(char16_t *restrict pc16,
                const char *restrict s,
                size_t n,
                mbstate_t *restrict ps)
{
    unsigned long value;
    size_t result;

    result = __uchar_import(&value, s, n, ps);
    if (pc16 != 0 && result != (size_t)-2) {
        *pc16 = (char16_t)value;
    }
    return result;
}

size_t c16rtomb(char *restrict s, char16_t c16, mbstate_t *restrict ps)
{
    return __uchar_export(s, (unsigned long)c16, ps);
}

size_t mbrtoc32(char32_t *restrict pc32,
                const char *restrict s,
                size_t n,
                mbstate_t *restrict ps)
{
    unsigned long value;
    size_t result;

    result = __uchar_import(&value, s, n, ps);
    if (pc32 != 0 && result != (size_t)-2) {
        *pc32 = (char32_t)value;
    }
    return result;
}

size_t c32rtomb(char *restrict s, char32_t c32, mbstate_t *restrict ps)
{
    return __uchar_export(s, (unsigned long)c32, ps);
}
