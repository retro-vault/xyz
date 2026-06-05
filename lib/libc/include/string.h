/*
 * string.h
 *
 * Standard C23 string and memory utilities.
 *
 * The implementations in this libc are byte-oriented and target the Z80's
 * flat 16-bit address space. Locale-sensitive entry points are presently
 * locale-neutral: strcoll performs the same ordering as strcmp, strxfrm uses
 * the identity transform, and strerror distinguishes only "no error" from a
 * generic failure string.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef __STRING_H__
#define __STRING_H__

#define __STDC_VERSION_STRING_H__ 202311L

#include <stddef.h>

/* Raw memory search, copy, move, compare, and fill primitives. */
extern void *memchr(const void *s, int c, size_t n);
extern void *memccpy(void *restrict s1, const void *restrict s2, int c, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
extern void *memmove(void *s1, const void *s2, size_t n);
extern void *memset(void *s, int c, size_t n);
extern void *memset_explicit(void *s, int c, size_t n);

/* NUL-terminated string copy and concatenation. */
extern char *strcat(char *restrict s1, const char *restrict s2);
extern char *strcpy(char *restrict s1, const char *restrict s2);
extern char *strncat(char *restrict s1, const char *restrict s2, size_t n);
extern char *strncpy(char *restrict s1, const char *restrict s2, size_t n);
extern char *strdup(const char *s);
extern char *strndup(const char *s, size_t n);

/* Lexicographic comparison and collation helpers. */
extern int strcmp(const char *s1, const char *s2);
extern int strcoll(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern size_t strxfrm(char *restrict s1, const char *restrict s2, size_t n);

/* String length and span calculations. */
extern size_t strlen(const char *s);
extern size_t strcspn(const char *s1, const char *s2);
extern size_t strnlen(const char *s, size_t n);
extern size_t strspn(const char *s1, const char *s2);

/* Character and substring search utilities. */
extern char *strchr(const char *s, int c);
extern char *strpbrk(const char *s1, const char *s2);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *s1, const char *s2);

/* Tokenization and diagnostic-message helpers. */
extern char *strtok(char *restrict s1, const char *restrict s2);
extern char *strerror(int errnum);

#endif /* __STRING_H__ */
