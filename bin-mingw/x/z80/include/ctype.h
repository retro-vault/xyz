/*
 * ctype.h
 *
 * Standard C23 character classification and case conversion.
 *
 * This libc currently implements the ctype family against the target's
 * single-byte ASCII execution character set. Each classifier accepts an int
 * promoted from unsigned char, or EOF-style sentinel values chosen by the
 * caller, and returns zero for false or nonzero for true.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef __CTYPE_H__
#define __CTYPE_H__

/* Character class predicates. */
extern int isalnum(int c);
extern int isalpha(int c);
extern int isblank(int c);
extern int iscntrl(int c);
extern int isdigit(int c);
extern int isgraph(int c);
extern int islower(int c);
extern int isprint(int c);
extern int ispunct(int c);
extern int isspace(int c);
extern int isupper(int c);
extern int isxdigit(int c);

/* Case conversion helpers. */
extern int tolower(int c);
extern int toupper(int c);

/* POSIX 7-bit ASCII helpers (implemented in assembly). */
extern int isascii(int c);
extern int toascii(int c);

#endif /* __CTYPE_H__ */
