/*
 * string.c
 *
 * minimal set of standard string functions.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 15.04.2021   tstih
 *
 */
#include "string.h"

addr_size_t string_length(char *str)
{
    addr_size_t length = 0;

    while (*str++)
        length++;

    return (length);
}

char *string_copy(char *dest, const char *src)
{
    addr_size_t index = 0;

    while (src[index])
    {
        dest[index] = src[index];
        index++;
    }

    return (dest);
}

int string_compare(char *s1, char *s2) {
    while (*s1 && *s2 && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return (*s1 - *s2);
}