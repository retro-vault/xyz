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

typedef int (*__libc_compare_fn)(const void *, const void *);

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
void *aligned_alloc(size_t alignment, size_t size);
size_t memalignment(const void *ptr);

/* C23 sized/aligned free (new) */
void free_sized(void *ptr, size_t size);
void free_aligned(void *ptr, size_t alignment);
void free_aligned_sized(void *ptr, size_t alignment, size_t size);

/*
 * Multi-heap allocator (xyz extension).
 *
 * Memory management lives in the platform layer: malloc()/free() are just
 * allocate()/deallocate() on the process-wide default heap, but a program may
 * create and use additional heaps (e.g. banked-memory blocks, or a separate OS
 * heap).  A heap is described by an 8-byte heap_t; pass its address as the
 * handle.  free() recovers a block's owning heap automatically, so a pointer
 * obtained from allocate(h, n) may be released with either deallocate(h, p) or
 * plain free(p).
 *
 * Create a heap over a memory region [base, limit) with heap_init_arena(); the
 * platform builds the default heap the same way on first malloc().
 */
typedef struct { unsigned char _opaque[8]; } heap_t;

extern heap_t _libc_default_heap;

void *allocate(heap_t *heap, size_t size);
void  deallocate(heap_t *heap, void *ptr);
void  heap_init_arena(heap_t *heap, void *base, void *limit);

int abs(int value);
long labs(long value);
long long llabs(long long value);

[[sdcc::sdccall(1)]] div_t div(int numer, int denom);
[[sdcc::sdccall(1)]] ldiv_t ldiv(long numer, long denom);
[[sdcc::sdccall(1)]] lldiv_t lldiv(long long numer, long long denom);

int atoi(const char *nptr);
double atof(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
float strtof(const char *restrict nptr, char **restrict endptr);
double strtod(const char *restrict nptr, char **restrict endptr);
long double strtold(const char *restrict nptr, char **restrict endptr);
long strtol(const char *restrict nptr, char **restrict endptr, int base);
unsigned long strtoul(const char *restrict nptr, char **restrict endptr, int base);
long long strtoll(const char *restrict nptr, char **restrict endptr, int base);
unsigned long long strtoull(const char *restrict nptr, char **restrict endptr, int base);

/* C23 float-to-string formatting (new) */
int strfromf(char *restrict s, size_t n, const char *restrict format, float fp);
int strfromd(char *restrict s, size_t n, const char *restrict format, double fp);
int strfroml(char *restrict s, size_t n, const char *restrict format, long double fp);

int rand(void);
void srand(unsigned int seed);

#if defined(__XCC__) && __SDCCCALL == 0
[[sdcc::sdccall(1)]] void *__bsearch_sdcc0(const void *key,
                                             const void *base,
                                             size_t count,
                                             size_t size,
                                             __libc_compare_fn compar);
[[sdcc::sdccall(1)]] void __qsort_sdcc0(void *base,
                                         size_t count,
                                         size_t size,
                                         __libc_compare_fn compar);
#define bsearch __bsearch_sdcc0
#define qsort   __qsort_sdcc0
#else
void *bsearch(const void *key,
              const void *base,
              size_t      count,
              size_t      size,
              __libc_compare_fn compar);
void qsort(void *base,
           size_t count,
           size_t size,
           __libc_compare_fn compar);
#endif

int mblen(const char *s, size_t n);
int mbtowc(wchar_t *restrict pwc, const char *restrict s, size_t n);
int wctomb(char *s, wchar_t wc);
size_t mbstowcs(wchar_t *restrict dst, const char *restrict src, size_t n);
size_t wcstombs(char *restrict dst, const wchar_t *restrict src, size_t n);

char *getenv(const char *name);
int system(const char *command);

#endif /* _STDLIB_H */
