#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define NULL ((void*)0)
typedef int (*__libc_compare_fn)(const void *, const void *);
typedef struct div_t { int quot; int rem; } div_t;
typedef struct ldiv_t { long quot; long rem; } ldiv_t;
typedef struct lldiv_t { long long quot; long long rem; } lldiv_t;

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
int atexit(void (*)(void));
int at_quick_exit(void (*)(void));
void quick_exit(int);
void _Exit(int);
void exit(int);
void abort(void);
int abs(int);
long labs(long);
long long llabs(long long);
div_t div(int, int);
ldiv_t ldiv(long, long);
lldiv_t lldiv(long long, long long);
int atoi(const char *);
double atof(const char *);
long atol(const char *);
long long atoll(const char *);
float strtof(const char *, char **);
double strtod(const char *, char **);
long double strtold(const char *, char **);
long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
long long strtoll(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);
int mblen(const char *, size_t);
int mbtowc(wchar_t *, const char *, size_t);
int wctomb(char *, wchar_t);
size_t mbstowcs(wchar_t *, const char *, size_t);
size_t wcstombs(char *, const wchar_t *, size_t);
char *getenv(const char *);
int system(const char *);
void *bsearch(const void *, const void *, size_t, size_t, __libc_compare_fn);
void qsort(void *, size_t, size_t, __libc_compare_fn);
int rand(void);
void srand(unsigned int);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

#endif
