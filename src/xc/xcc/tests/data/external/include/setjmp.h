#ifndef _SETJMP_H
#define _SETJMP_H

typedef int jmp_buf[8];

int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif
