#ifndef _STDIO_H
#define _STDIO_H

typedef struct _FILE FILE;
typedef long fpos_t;
#define NULL ((void*)0)
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 256
#define L_tmpnam 12
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, unsigned int, const char *, ...);
int vprintf(const char *, char *);
int vfprintf(FILE *, const char *, char *);
int vsprintf(char *, const char *, char *);
int vsnprintf(char *, unsigned int, const char *, char *);
int scanf(const char *, ...);
int fscanf(FILE *, const char *, ...);
int sscanf(const char *, const char *, ...);
int vscanf(const char *, char *);
int vfscanf(FILE *, const char *, char *);
int vsscanf(const char *, const char *, char *);
int putchar(int);
int puts(const char *);
int getchar(void);
int fgetc(FILE *);
int getc(FILE *);
int fputc(int, FILE *);
int putc(int, FILE *);
int fputs(const char *, FILE *);
int ungetc(int, FILE *);
char *fgets(char *, int, FILE *);
FILE *fopen(const char *, const char *);
FILE *freopen(const char *, const char *, FILE *);
int fclose(FILE *);
FILE *tmpfile(void);
int fseek(FILE *, long, int);
long ftell(FILE *);
int fgetpos(FILE *, fpos_t *);
int fsetpos(FILE *, const fpos_t *);
void rewind(FILE *);
int feof(FILE *);
int ferror(FILE *);
void clearerr(FILE *);
int remove(const char *);
int rename(const char *, const char *);
char *tmpnam(char *);
void perror(const char *);
void setbuf(FILE *, char *);
int setvbuf(FILE *, char *, int, unsigned int);
unsigned int fread(void *, unsigned int, unsigned int, FILE *);
unsigned int fwrite(const void *, unsigned int, unsigned int, FILE *);
int fflush(FILE *);

#endif
