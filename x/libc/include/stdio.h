/*
 * stdio.h
 *
 * Small unbuffered stdio subset for the xcc Z80 libc.
 *
 * This implementation provides the classic formatted-output family plus a
 * small fd-backed input/block-I/O layer. FILE handles are currently tiny
 * descriptors for stdin/stdout/stderr-style use and sit on top of Unix-like
 * read/write calls supplied by the platform layer.
 *
 * The true variadic stdio entry points use the stack-only sdcccall(0) ABI.
 * Fixed-argument stdio calls keep the normal ABI.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDIO_H
#define _STDIO_H

#define __STDC_VERSION_STDIO_H__ 202311L

#include <stddef.h>
#include <stdarg.h>

typedef struct __stdio_file FILE;
typedef long fpos_t;

#ifndef EOF
#  define EOF (-1)
#endif

#ifndef BUFSIZ
#  define BUFSIZ 256
#endif

#ifndef L_tmpnam
#  define L_tmpnam 12
#endif

#ifndef FILENAME_MAX
#  define FILENAME_MAX 255
#endif

#ifndef _IOFBF
#  define _IOFBF 0
#  define _IOLBF 1
#  define _IONBF 2
#endif

#ifndef SEEK_SET
#  define SEEK_SET 0
#  define SEEK_CUR 1
#  define SEEK_END 2
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int c);
int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int puts(const char *s);
int fputs(const char *s, FILE *stream);
int getchar(void);
int fgetc(FILE *stream);
int getc(FILE *stream);
int ungetc(int c, FILE *stream);
FILE *fopen(const char *restrict path,
            const char *restrict mode);
FILE *freopen(const char *restrict path,
              const char *restrict mode,
              FILE *restrict stream);
int fclose(FILE *stream);
FILE *tmpfile(void);
char *fgets(char *restrict s, int n,
            FILE *restrict stream);
size_t fread(void *restrict ptr, size_t size,
             size_t nmemb, FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size,
              size_t nmemb, FILE *restrict stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fgetpos(FILE *restrict stream,
            fpos_t *restrict pos);
int fsetpos(FILE *stream, const fpos_t *pos);
void rewind(FILE *stream);
int fflush(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int remove(const char *path);
int rename(const char *old_path, const char *new_path);
char *tmpnam(char *s);
void perror(const char *s);
void setbuf(FILE *restrict stream, char *restrict buf);
int setvbuf(FILE *restrict stream,
            char *restrict buf,
            int mode,
            size_t size);

[[sdcc::sdccall(0)]] int printf(const char *restrict format, ...);
[[sdcc::sdccall(0)]] int fprintf(FILE *restrict stream,
                                 const char *restrict format, ...);
[[sdcc::sdccall(0)]] int sprintf(char *restrict s,
                                 const char *restrict format, ...);
[[sdcc::sdccall(0)]] int snprintf(char *restrict s, size_t n,
                                  const char *restrict format, ...);
[[sdcc::sdccall(0)]] int scanf(const char *restrict format, ...);
[[sdcc::sdccall(0)]] int fscanf(FILE *restrict stream,
                                const char *restrict format, ...);
[[sdcc::sdccall(0)]] int sscanf(const char *restrict s,
                                const char *restrict format, ...);

int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream,
             const char *restrict format, va_list ap);
int vsprintf(char *restrict s,
             const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n,
              const char *restrict format, va_list ap);
int vscanf(const char *restrict format, va_list ap);
int vfscanf(FILE *restrict stream,
            const char *restrict format, va_list ap);
int vsscanf(const char *restrict s,
            const char *restrict format, va_list ap);

#endif /* _STDIO_H */
