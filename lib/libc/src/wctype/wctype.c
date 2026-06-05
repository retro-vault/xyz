/*
 * wctype.c
 *
 * Wide-character classification support for the xcc Z80 libc.
 *
 * The current target maps wide-character classification onto the existing
 * ASCII ctype layer whenever the input fits in the unsigned-byte range.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <wctype.h>

#define __WCTYPE_NONE    0U
#define __WCTYPE_ALNUM   1U
#define __WCTYPE_ALPHA   2U
#define __WCTYPE_BLANK   3U
#define __WCTYPE_CNTRL   4U
#define __WCTYPE_DIGIT   5U
#define __WCTYPE_GRAPH   6U
#define __WCTYPE_LOWER   7U
#define __WCTYPE_PRINT   8U
#define __WCTYPE_PUNCT   9U
#define __WCTYPE_SPACE   10U
#define __WCTYPE_UPPER   11U
#define __WCTYPE_XDIGIT  12U

#define __WCTRANS_NONE   0U
#define __WCTRANS_TOLOWER 1U
#define __WCTRANS_TOUPPER 2U

static int __wctype_byte_value(wint_t wc)
{
    if (wc > UCHAR_MAX) {
        return -1;
    }
    return (int)wc;
}

wint_t towlower(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? wc : (wint_t)(unsigned char)tolower(value);
}

wint_t towupper(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? wc : (wint_t)(unsigned char)toupper(value);
}

int iswalnum(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isalnum(value);
}

int iswalpha(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isalpha(value);
}

int iswblank(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isblank(value);
}

int iswcntrl(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : iscntrl(value);
}

int iswdigit(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isdigit(value);
}

int iswgraph(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isgraph(value);
}

int iswlower(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : islower(value);
}

int iswprint(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isprint(value);
}

int iswpunct(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : ispunct(value);
}

int iswspace(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isspace(value);
}

int iswupper(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isupper(value);
}

int iswxdigit(wint_t wc)
{
    int value;

    value = __wctype_byte_value(wc);
    return value < 0 ? 0 : isxdigit(value);
}

wctype_t wctype(const char *name)
{
    if (name == 0) {
        return __WCTYPE_NONE;
    }
    if (strcmp(name, "alnum") == 0) {
        return __WCTYPE_ALNUM;
    }
    if (strcmp(name, "alpha") == 0) {
        return __WCTYPE_ALPHA;
    }
    if (strcmp(name, "blank") == 0) {
        return __WCTYPE_BLANK;
    }
    if (strcmp(name, "cntrl") == 0) {
        return __WCTYPE_CNTRL;
    }
    if (strcmp(name, "digit") == 0) {
        return __WCTYPE_DIGIT;
    }
    if (strcmp(name, "graph") == 0) {
        return __WCTYPE_GRAPH;
    }
    if (strcmp(name, "lower") == 0) {
        return __WCTYPE_LOWER;
    }
    if (strcmp(name, "print") == 0) {
        return __WCTYPE_PRINT;
    }
    if (strcmp(name, "punct") == 0) {
        return __WCTYPE_PUNCT;
    }
    if (strcmp(name, "space") == 0) {
        return __WCTYPE_SPACE;
    }
    if (strcmp(name, "upper") == 0) {
        return __WCTYPE_UPPER;
    }
    if (strcmp(name, "xdigit") == 0) {
        return __WCTYPE_XDIGIT;
    }
    return __WCTYPE_NONE;
}

int iswctype(wint_t wc, wctype_t desc)
{
    switch (desc) {
    case __WCTYPE_ALNUM:
        return iswalnum(wc);
    case __WCTYPE_ALPHA:
        return iswalpha(wc);
    case __WCTYPE_BLANK:
        return iswblank(wc);
    case __WCTYPE_CNTRL:
        return iswcntrl(wc);
    case __WCTYPE_DIGIT:
        return iswdigit(wc);
    case __WCTYPE_GRAPH:
        return iswgraph(wc);
    case __WCTYPE_LOWER:
        return iswlower(wc);
    case __WCTYPE_PRINT:
        return iswprint(wc);
    case __WCTYPE_PUNCT:
        return iswpunct(wc);
    case __WCTYPE_SPACE:
        return iswspace(wc);
    case __WCTYPE_UPPER:
        return iswupper(wc);
    case __WCTYPE_XDIGIT:
        return iswxdigit(wc);
    default:
        return 0;
    }
}

wctrans_t wctrans(const char *name)
{
    if (name == 0) {
        return __WCTRANS_NONE;
    }
    if (strcmp(name, "tolower") == 0) {
        return __WCTRANS_TOLOWER;
    }
    if (strcmp(name, "toupper") == 0) {
        return __WCTRANS_TOUPPER;
    }
    return __WCTRANS_NONE;
}

wint_t towctrans(wint_t wc, wctrans_t desc)
{
    switch (desc) {
    case __WCTRANS_TOLOWER:
        return towlower(wc);
    case __WCTRANS_TOUPPER:
        return towupper(wc);
    default:
        return wc;
    }
}
