/*
 * string.h
 *
 * (minimal!) implementation of string.h
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.07.2021   tstih
 *
 */
#include <string.h>

size_t strlen(const char *s)
{
    size_t l = 0;
    while (*s++) 
        l++;
    return (l);
}

char *strcpy(char *d, const char *s)
{
    size_t i = 0;
    while (s[i])
    {
        d[i] = s[i];
        i++;
    }
    return (d);
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return (*s1 - *s2);
}