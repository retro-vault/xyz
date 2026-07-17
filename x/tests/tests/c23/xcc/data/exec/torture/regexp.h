/* Portable test-only interface for Henry Spencer's V8 regexp implementation. */
#ifndef XCC_TORTURE_REGEXP_H
#define XCC_TORTURE_REGEXP_H

#define NSUBEXP 10

typedef struct regexp {
    char *startp[NSUBEXP];
    char *endp[NSUBEXP];
    char regstart;
    char reganch;
    char *regmust;
    int regmlen;
    char program[1];
} regexp;

regexp *regcomp(char *expression);
int regexec(regexp *program, char *string);
void regerror(char *message);

#endif
