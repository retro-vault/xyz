/*
 * ctype.h
 *
 * minimal standard C ctype.h
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 09.07.2021   tstih
 *
 */
#ifndef __CTYPE_H__
#define __CTYPE_H__

#include <stdbool.h>

/* True if char is a letter. */
extern bool isalpha(int c);

/* True if char is white space. */
extern bool isspace(int c);

/* Returns char, converted to lowercase. */
extern int tolower(int c);

#endif /* __CTYPE_H__ */