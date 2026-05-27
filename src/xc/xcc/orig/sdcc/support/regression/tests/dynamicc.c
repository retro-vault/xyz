/** dynamicc.c - test Dynamic C calling convention support, mostly parameter passing.
*/
#include <testfwk.h>

#if !defined(__SDCC_r2k) && !defined(__SDCC_r2ka) && !defined(__SDCC_r3ka) && !defined(__SDCC_r4k) && !defined(__SDCC_r5k) && !defined(__SDCC_r6k) && !defined(__SDCC_tlcs90) && !defined(__SDCC_ez80)
#define __far
#endif

char passChar(char p) __dynamicc
{
	return p + 1;
}

int passInt(int p) __dynamicc
{
	return p + 1;
}

long passLong(long p) __dynamicc
{
	return p + 1;
}

char *passPtr(char *p) __dynamicc
{
	return p + 1;
}

struct s
{
	int i;
};

int passStruct(struct s p) __dynamicc // first parameter on stack.
{
	return p.i + 1;
}

char *passFarPtr(__far char *p) __dynamicc // first parameter on stack, return value would be in px if pointer to __far.
{
	return (char *)p + 1;
}

char passChar2(char p, char p2) __dynamicc
{
	return p + p2;
}

int passInt2(int p, int p2) __dynamicc // first parameter in hl and on stack, return value in hl.
{
	return p + p2;
}

long passLong2(long p, long p2) __dynamicc // first parameter in bcde and on stack, return value in bcde.
{
	return p + p2;
}

char *passPtr2(char *p, int p2) __dynamicc
{
	return p + p2;
}

int passStruct2(struct s p, int p2) __dynamicc
{
	return p.i + p2;
}

__far char fc, fd;

int passFarPtr2(__far char *p, __far char *q) __dynamicc // first parameter on stack, return value would be in px if pointer to __far.
{
	return (p == &fc && q == &fd);
}

volatile char c = 0x5a;
volatile int i = 0x55aa;
volatile long l = 0x5aa555aa;
struct s s = {0xa5};

void testDynamicC(void)
{
	ASSERT(passChar(c) == c + 1);
	ASSERT(passInt(i) == i + 1);
	ASSERT(passLong(l) == l + 1);
	ASSERT(passPtr(&c) == &c + 1);
	ASSERT(passStruct(s) == s.i + 1);
	ASSERT(passFarPtr((__far char*)(&c)) == &c + 1);

	ASSERT(passChar2(c, 2) == c + 2);
	ASSERT(passInt2(i, 2) == i + 2);
	ASSERT(passLong2(l, 2) == l + 2);
	ASSERT(passPtr2(&c, 1) == &c + 1);
	ASSERT(passStruct2(s, 2) == s.i + 2);
	ASSERT(passFarPtr2(&fc, &fd));
}

