/*
 * wchar.c
 *
 * Wide-character string and memory primitives for the xcc Z80 libc.
 *
 * The current target uses 16-bit wchar_t values and a stateless single-byte
 * execution encoding. The helpers below therefore focus on the platform-
 * independent wide string and wide memory subset.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <wchar.h>

static int __wchar_is_delim(wchar_t ch, const wchar_t *delim)
{
    while (*delim != 0) {
        if (*delim == ch) {
            return 1;
        }
        ++delim;
    }
    return 0;
}

wint_t btowc(int c)
{
    if (c < 0 || c > 255) {
        return WEOF;
    }
    return (wint_t)(unsigned char)c;
}

int wctob(wint_t c)
{
    if (c > 255U) {
        return -1;
    }
    return (int)c;
}

int mbsinit(const mbstate_t *ps)
{
    (void)ps;
    return 1;
}

wchar_t *wcscat(wchar_t *restrict dst, const wchar_t *restrict src)
{
    wchar_t *out;

    out = dst + wcslen(dst);
    while (*src != 0) {
        *out++ = *src++;
    }
    *out = 0;
    return dst;
}

wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
    while (*s != 0) {
        if (*s == c) {
            return (wchar_t *)s;
        }
        ++s;
    }
    return c == 0 ? (wchar_t *)s : 0;
}

int wcscmp(const wchar_t *lhs, const wchar_t *rhs)
{
    while (*lhs != 0 && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }
    if (*lhs < *rhs) {
        return -1;
    }
    if (*lhs > *rhs) {
        return 1;
    }
    return 0;
}

wchar_t *wcscpy(wchar_t *restrict dst, const wchar_t *restrict src)
{
    wchar_t *out;

    out = dst;
    while ((*out++ = *src++) != 0) {
    }
    return dst;
}

size_t wcscspn(const wchar_t *s, const wchar_t *reject)
{
    size_t count;

    count = 0U;
    while (*s != 0) {
        if (__wchar_is_delim(*s, reject)) {
            break;
        }
        ++s;
        ++count;
    }
    return count;
}

size_t wcslen(const wchar_t *s)
{
    const wchar_t *start;

    start = s;
    while (*s != 0) {
        ++s;
    }
    return (size_t)(s - start);
}

wchar_t *wcsncat(wchar_t *restrict dst, const wchar_t *restrict src, size_t count)
{
    wchar_t *out;

    out = dst + wcslen(dst);
    while (count != 0U && *src != 0) {
        *out++ = *src++;
        --count;
    }
    *out = 0;
    return dst;
}

int wcsncmp(const wchar_t *lhs, const wchar_t *rhs, size_t count)
{
    while (count != 0U && *lhs != 0 && *lhs == *rhs) {
        ++lhs;
        ++rhs;
        --count;
    }
    if (count == 0U) {
        return 0;
    }
    if (*lhs < *rhs) {
        return -1;
    }
    if (*lhs > *rhs) {
        return 1;
    }
    return 0;
}

wchar_t *wcsncpy(wchar_t *restrict dst, const wchar_t *restrict src, size_t count)
{
    wchar_t *out;

    out = dst;
    while (count != 0U && *src != 0) {
        *out++ = *src++;
        --count;
    }
    while (count != 0U) {
        *out++ = 0;
        --count;
    }
    return dst;
}

size_t wcsnlen(const wchar_t *s, size_t count)
{
    size_t size;

    size = 0U;
    while (count != 0U && *s != 0) {
        ++s;
        ++size;
        --count;
    }
    return size;
}

wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept)
{
    while (*s != 0) {
        if (__wchar_is_delim(*s, accept)) {
            return (wchar_t *)s;
        }
        ++s;
    }
    return 0;
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t c)
{
    wchar_t *match;

    match = 0;
    while (*s != 0) {
        if (*s == c) {
            match = (wchar_t *)s;
        }
        ++s;
    }
    return c == 0 ? (wchar_t *)s : match;
}

size_t wcsspn(const wchar_t *s, const wchar_t *accept)
{
    size_t count;

    count = 0U;
    while (*s != 0) {
        if (!__wchar_is_delim(*s, accept)) {
            break;
        }
        ++s;
        ++count;
    }
    return count;
}

wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle)
{
    size_t needle_len;

    needle_len = wcslen(needle);
    if (needle_len == 0U) {
        return (wchar_t *)haystack;
    }

    while (*haystack != 0) {
        if (wcsncmp(haystack, needle, needle_len) == 0) {
            return (wchar_t *)haystack;
        }
        ++haystack;
    }
    return 0;
}

wchar_t *wcstok(wchar_t *restrict s, const wchar_t *restrict delim, wchar_t **restrict state)
{
    wchar_t *start;

    if (s == 0) {
        if (state == 0) {
            return 0;
        }
        s = *state;
    }
    if (s == 0) {
        return 0;
    }

    while (*s != 0 && __wchar_is_delim(*s, delim)) {
        ++s;
    }
    if (*s == 0) {
        if (state != 0) {
            *state = 0;
        }
        return 0;
    }

    start = s;
    while (*s != 0 && !__wchar_is_delim(*s, delim)) {
        ++s;
    }
    if (*s != 0) {
        *s++ = 0;
    }
    if (state != 0) {
        *state = s;
    }
    return start;
}

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t count)
{
    while (count != 0U) {
        if (*s == c) {
            return (wchar_t *)s;
        }
        ++s;
        --count;
    }
    return 0;
}

int wmemcmp(const wchar_t *lhs, const wchar_t *rhs, size_t count)
{
    while (count != 0U) {
        if (*lhs < *rhs) {
            return -1;
        }
        if (*lhs > *rhs) {
            return 1;
        }
        ++lhs;
        ++rhs;
        --count;
    }
    return 0;
}

wchar_t *wmemcpy(wchar_t *restrict dst, const wchar_t *restrict src, size_t count)
{
    wchar_t *out;

    out = dst;
    while (count != 0U) {
        *out++ = *src++;
        --count;
    }
    return dst;
}

wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t count)
{
    wchar_t *result;

    result = dst;
    if (dst == src || count == 0U) {
        return result;
    }
    if (dst < src) {
        while (count != 0U) {
            *dst++ = *src++;
            --count;
        }
    } else {
        dst += count;
        src += count;
        while (count != 0U) {
            --dst;
            --src;
            *dst = *src;
            --count;
        }
    }
    return result;
}

wchar_t *wmemset(wchar_t *dst, wchar_t c, size_t count)
{
    wchar_t *out;

    out = dst;
    while (count != 0U) {
        *out++ = c;
        --count;
    }
    return dst;
}
