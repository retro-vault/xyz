/*
 * locale.c
 *
 * Built-in "C" locale support for the xcc Z80 libc.
 *
 * This target currently exposes just one locale profile. setlocale() accepts
 * the standard category values but can only select or report the built-in "C"
 * locale, and localeconv() returns the matching static formatting data.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <limits.h>
#include <locale.h>
#include <string.h>

static char __locale_name[] = "C";

static struct lconv __locale_c = {
    ".",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX
};

static int __locale_category_is_valid(int category)
{
    return category == LC_ALL ||
           category == LC_COLLATE ||
           category == LC_CTYPE ||
           category == LC_MONETARY ||
           category == LC_NUMERIC ||
           category == LC_TIME;
}

char *setlocale(int category, const char *locale)
{
    if (!__locale_category_is_valid(category)) {
        return 0;
    }

    if (locale == 0) {
        return __locale_name;
    }

    if (locale[0] == '\0' ||
        strcmp(locale, "C") == 0 ||
        strcmp(locale, "POSIX") == 0) {
        return __locale_name;
    }

    return 0;
}

struct lconv *localeconv(void)
{
    return &__locale_c;
}
