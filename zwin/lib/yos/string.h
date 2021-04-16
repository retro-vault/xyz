/*
 * string.h
 *
 * minimal set of standard string functions.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 15.04.2021   tstih
 *
 */
#ifndef _STRING_H
#define _STRING_H

#include "yos.h"

extern addr_size_t string_length(char *str);
extern char *string_copy(char *dest, const char *src);
extern int string_compare(char *s1, char *s2);

#endif /* _STRING_H */