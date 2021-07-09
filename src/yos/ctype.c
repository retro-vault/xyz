/*
 * ctype.c
 *
 * minimal standard C ctype.h implementation
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 09.07.2021   tstih
 *
 */
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

bool isalpha(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isspace(int c)
{
    return c == ' ' || c == '\t';
}

int tolower(int c)
{
    if (!isalpha(c))
        return c;

    if (c >= 'A' && c <= 'Z') 
        return c + 32;
    else
        return c;
}