/*
 * locale.h
 *
 * Standard C23 locale selection support for the xcc Z80 target.
 *
 * The current libc exposes one built-in "C" locale. setlocale() can select or
 * query that locale, and localeconv() returns a static description of its
 * numeric and monetary formatting fields.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _LOCALE_H
#define _LOCALE_H

#define __STDC_VERSION_LOCALE_H__ 202311L

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char  int_frac_digits;
    char  frac_digits;
    char  p_cs_precedes;
    char  p_sep_by_space;
    char  n_cs_precedes;
    char  n_sep_by_space;
    char  p_sign_posn;
    char  n_sign_posn;
    char  int_p_cs_precedes;
    char  int_p_sep_by_space;
    char  int_n_cs_precedes;
    char  int_n_sep_by_space;
    char  int_p_sign_posn;
    char  int_n_sign_posn;
};

#define LC_ALL       0
#define LC_COLLATE   1
#define LC_CTYPE     2
#define LC_MONETARY  3
#define LC_NUMERIC   4
#define LC_TIME      5

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

#endif /* _LOCALE_H */
