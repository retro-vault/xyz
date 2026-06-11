/*
 * uchar.h
 *
 * Standard C23 UTF-16 and UTF-32 conversion support for the xcc Z80 target.
 *
 * The current libc uses the target's single-byte execution encoding as its
 * multibyte form. Conversion is therefore stateless and maps one byte to one
 * code unit when the value lies in the unsigned-byte range.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _UCHAR_H
#define _UCHAR_H

#define __STDC_VERSION_UCHAR_H__ 202311L

#include <stddef.h>
#include <wchar.h>

typedef unsigned short char16_t;
typedef unsigned long  char32_t;

/* C23 UTF-8 support (char8_t and conversion functions).
 * On this target the execution charset is single-byte, so these are
 * essentially 1:1 mappings for values in [0,255]. */
typedef unsigned char char8_t;

size_t mbrtoc8(char8_t *restrict pc8,
               const char *restrict s,
               size_t n,
               mbstate_t *restrict ps);
size_t c8rtomb(char *restrict s, char8_t c8, mbstate_t *restrict ps);

size_t mbrtoc16(char16_t *restrict pc16,
                const char *restrict s,
                size_t n,
                mbstate_t *restrict ps);
size_t c16rtomb(char *restrict s, char16_t c16, mbstate_t *restrict ps);

size_t mbrtoc32(char32_t *restrict pc32,
                const char *restrict s,
                size_t n,
                mbstate_t *restrict ps);
size_t c32rtomb(char *restrict s, char32_t c32, mbstate_t *restrict ps);

#endif /* _UCHAR_H */
