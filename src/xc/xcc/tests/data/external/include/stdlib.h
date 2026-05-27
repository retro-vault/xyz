#ifndef _STDLIB_H
#define _STDLIB_H

#define NULL ((void*)0)
typedef unsigned int size_t;

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
void exit(int);
void abort(void);
int abs(int);
long labs(long);
int atoi(const char *);
long atol(const char *);
long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
void *bsearch(const void *, const void *, size_t, size_t,
              int (*)(const void *, const void *));
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int rand(void);
void srand(unsigned int);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

#endif
