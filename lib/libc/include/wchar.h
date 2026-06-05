/*
 * wchar.h
 *
 * Standard C23 wide-character support for the xcc Z80 target.
 *
 * The current libc provides the target-independent wide string and wide memory
 * primitives plus simple single-byte conversion helpers for the execution
 * character set. Wide I/O and locale-dependent formatting remain outside this
 * first runtime slice.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _WCHAR_H
#define _WCHAR_H

#define __STDC_VERSION_WCHAR_H__ 202311L

#include <stddef.h>

typedef unsigned int wint_t;
typedef struct mbstate_t {
    unsigned int __unused;
} mbstate_t;

#define WEOF ((wint_t)0xFFFFU)

wint_t btowc(int c);
int wctob(wint_t c);
int mbsinit(const mbstate_t *ps);

wchar_t *wcscat(wchar_t *restrict dst, const wchar_t *restrict src);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
int wcscmp(const wchar_t *lhs, const wchar_t *rhs);
wchar_t *wcscpy(wchar_t *restrict dst, const wchar_t *restrict src);
size_t wcscspn(const wchar_t *s, const wchar_t *reject);
size_t wcslen(const wchar_t *s);
wchar_t *wcsncat(wchar_t *restrict dst, const wchar_t *restrict src, size_t count);
int wcsncmp(const wchar_t *lhs, const wchar_t *rhs, size_t count);
wchar_t *wcsncpy(wchar_t *restrict dst, const wchar_t *restrict src, size_t count);
size_t wcsnlen(const wchar_t *s, size_t count);
wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
size_t wcsspn(const wchar_t *s, const wchar_t *accept);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
wchar_t *wcstok(wchar_t *restrict s, const wchar_t *restrict delim, wchar_t **restrict state);

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t count);
int wmemcmp(const wchar_t *lhs, const wchar_t *rhs, size_t count);
wchar_t *wmemcpy(wchar_t *restrict dst, const wchar_t *restrict src, size_t count);
wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t count);
wchar_t *wmemset(wchar_t *dst, wchar_t c, size_t count);

#endif /* _WCHAR_H */
