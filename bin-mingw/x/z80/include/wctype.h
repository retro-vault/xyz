/*
 * wctype.h
 *
 * Standard C23 wide-character classification and mapping support for the xcc
 * Z80 target.
 *
 * The current implementation applies the existing ASCII ctype rules to
 * wide-character values in the unsigned-byte range and reports false for wider
 * code points. This keeps the wide classification layer honest until fuller
 * Unicode tables are introduced.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _WCTYPE_H
#define _WCTYPE_H

#define __STDC_VERSION_WCTYPE_H__ 202311L

#include <wchar.h>

typedef unsigned short wctype_t;
typedef unsigned short wctrans_t;

wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);
int iswalnum(wint_t wc);
int iswalpha(wint_t wc);
int iswblank(wint_t wc);
int iswcntrl(wint_t wc);
int iswdigit(wint_t wc);
int iswgraph(wint_t wc);
int iswlower(wint_t wc);
int iswprint(wint_t wc);
int iswpunct(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswxdigit(wint_t wc);

wctype_t wctype(const char *name);
int iswctype(wint_t wc, wctype_t desc);
wctrans_t wctrans(const char *name);
wint_t towctrans(wint_t wc, wctrans_t desc);

#endif /* _WCTYPE_H */
