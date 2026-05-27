#ifndef _STDIO_H
#define _STDIO_H

typedef struct _FILE FILE;
#define NULL ((void*)0)
#define EOF (-1)

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, unsigned int, const char *, ...);
int scanf(const char *, ...);
int fscanf(FILE *, const char *, ...);
int sscanf(const char *, const char *, ...);
int putchar(int);
int puts(const char *);
int getchar(void);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
int feof(FILE *);
unsigned int fread(void *, unsigned int, unsigned int, FILE *);
unsigned int fwrite(const void *, unsigned int, unsigned int, FILE *);
int fflush(FILE *);

#endif
