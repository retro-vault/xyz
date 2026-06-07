/*
 * strings.h
 *
 * Legacy BSD string and byte-array helpers for the xcc Z80 libc.
 * These complement <string.h>; the case-insensitive comparison routines are
 * declared here (as on BSD) and also in <string.h> for convenience.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <stddef.h>

/* Byte-array operations (legacy BSD). */
extern void  bcopy(const void *src, void *dest, size_t n);
extern void  bzero(void *s, size_t n);
extern int   bcmp(const void *s1, const void *s2, size_t n);

/* Character search aliases (legacy BSD). */
extern char *index(const char *s, int c);
extern char *rindex(const char *s, int c);

/* Case-insensitive comparison (POSIX). */
extern int   strcasecmp(const char *s1, const char *s2);
extern int   strncasecmp(const char *s1, const char *s2, size_t n);

/* Find first set bit (POSIX / GNU). */
extern int   ffs(int i);
extern int   ffsl(long i);
extern int   ffsll(long long i);

#endif /* __STRINGS_H__ */
