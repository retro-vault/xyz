/* bug-3712.c
   A caller-side bug in mcs51 peephole optimizer notUsed when a non-bit parameter was preceded by a bit parameter and a calle-side bug in stack setup for such functions.
*/

#include <testfwk.h>

#if !defined(__SDCC_mcs51) && !defined(__SDCC_ds390)
#define __bit _Bool
#endif

void f1(__bit b, unsigned char c) __reentrant;
void f4(__bit b, unsigned long c) __reentrant;
void f8(__bit b, unsigned long long l) __reentrant;

void g(void)
{
	f1(0, 0x55);
	f4(0, 0x55aa5aa5);
	f8(0, 0x55aa5aa501020304);
}

void
testBug(void)
{
	g();
}

void f1(__bit b, unsigned char c) __reentrant
{
	ASSERT (b == 0);
	ASSERT (c == 0x55);
}

void f4(__bit b, unsigned long l) __reentrant
{
	ASSERT (b == 0);
	ASSERT (l == 0x55aa5aa5);
}

void f8(__bit b, unsigned long long l) __reentrant
{
	ASSERT (b == 0);
	ASSERT (l == 0x55aa5aa501020304);
}

