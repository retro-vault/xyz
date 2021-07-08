/*
 * string.h
 *
 * (minimal!) standard C header file for strings
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.07.2021   tstih
 *
 */
#ifndef __STRING_H__
#define __STRING_H___

#include <stddef.h>

/* Zero terminated string length. */
extern size_t strlen(const char *s);

/* Copy string to another string */
extern char *strcpy(char *d, const char *s);

/* Compare strings, 0=match. */
extern int strcmp(const char *s1, const char *s2);

#endif /* __STRING_H__ */