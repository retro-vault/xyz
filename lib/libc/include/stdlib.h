/*
 * stdlib.h
 *
 * Standard C23 general utilities for the xcc Z80 target.
 *
 * This libc currently provides the target-independent subset of stdlib: a
 * small in-library heap, integer conversion routines, absolute-value and div
 * helpers, qsort/bsearch, simple rand/srand, and minimal process-termination
 * hooks.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDLIB_H
#define _STDLIB_H

#define __STDC_VERSION_STDLIB_H__ 202311L

#include <stddef.h>

typedef struct div_t {
    int quot;
    int rem;
} div_t;

typedef struct ldiv_t {
    long quot;
    long rem;
} ldiv_t;

typedef struct lldiv_t {
    long long quot;
    long long rem;
} lldiv_t;

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     32767
#define MB_CUR_MAX   1

_Noreturn void abort(void);
int atexit(void (*func)(void));
_Noreturn void exit(int status);
_Noreturn void _Exit(int status);
int at_quick_exit(void (*func)(void));
_Noreturn void quick_exit(int status);

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

int abs(int value);
long labs(long value);
long long llabs(long long value);

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
long strtol(const char *restrict nptr, char **restrict endptr, int base);
unsigned long strtoul(const char *restrict nptr, char **restrict endptr, int base);
long long strtoll(const char *restrict nptr, char **restrict endptr, int base);
unsigned long long strtoull(const char *restrict nptr, char **restrict endptr, int base);

int rand(void);
void srand(unsigned int seed);

void *bsearch(const void *key,
              const void *base,
              size_t      count,
              size_t      size,
              int (*compar)(const void *, const void *));
void qsort(void *base,
           size_t count,
           size_t size,
           int (*compar)(const void *, const void *));

#endif /* _STDLIB_H */
