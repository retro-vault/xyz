/** dynamiccasmchk.c - test Dynamic C calling convention support by calling asm-implemented routines
*/
#include <testfwk.h>

volatile char c = 0x5a;
volatile int i = 0x55aa;
volatile long l = 0x5aa555aa;

// For ports that support the Dynamic C calling convention test the passing of the first parameter in both registers and stack. Leave out sm83 and tlcs870 for now, since their restricted instruction set makes it harder to write the tests.
#if defined (__SDCC_z80) || defined (__SDCC_z80n) || defined (__SDCC_z180) || defined (__SDCC_r2k) || defined (__SDCC_r2ka) || defined (__SDCC_r3ka) || defined (__SDCC_r4k) || defined (__SDCC_r5k) || defined (__SDCC_r6k) || defined (__SDCC_tlcs90) || defined (__SDCC_tlcs870c) || defined (__SDCC_tlcs870c1) || defined (__SDCC_ez80) || defined (__SDCC_r800)

#pragma disable_warning 85

unsigned char passChar(char p) __dynamicc __naked
{
__asm
	ld	ix, #0xabcd
	ld	a, (_c)
	cp	a, l
	jr	nz, bad_char
	pop	bc
	pop	hl
	push	hl
	push	bc
	cp	a, l
	jr	nz, bad_char
	ld	hl, #1
	ret
bad_char:
	ld	hl, #0
	ret
__endasm;
}

unsigned char passInt(int p) __dynamicc __naked
{
__asm
	ld	ix, #0xabcd
	ex	de, hl
	ld	hl, (_i)
	cp	a, a
	sbc	hl, de
	jr	nz, bad_int
	pop	bc
	pop	hl
	push	hl
	push	bc
	cp	a, a
	sbc	hl, de
	jr	nz, bad_int
	ld	hl, #1
	ret
bad_int:
	ld	hl, #0
	ret
__endasm;
}

unsigned char passLong(long p) __dynamicc __naked
{
__asm
	ld	ix, #0xabcd
	ld	hl, (_l)
	cp	a, a
	sbc	hl, de
	jr	nz, bad_long
	ld	hl, (_l+2)
	cp	a, a
	sbc	hl, bc
	jr	nz, bad_long
	pop	iy
	pop	hl
	push	hl
	push	iy
	cp	a, a
	sbc	hl, de
	jr	nz, bad_long
	pop	iy
	pop	de
	pop	hl
	push	hl
	push	de
	push	iy
	cp	a, a
	sbc	hl, bc
	jr	nz, bad_long
	ld	hl, #1
	ret
bad_long:
	ld	hl, #0
	ret
__endasm;
}

unsigned char passPtr(char *p) __dynamicc __naked
{
__asm
	ld	ix, #0xabcd
	ex	de, hl
	ld	hl, #_c
	cp	a, a
	sbc	hl, de
	jr	nz, bad_ptr
	pop	bc
	pop	hl
	push	hl
	push	bc
	cp	a, a
	sbc	hl, de
	jr	nz, bad_ptr
	ld	hl, #1
	ret
bad_ptr:
	ld	hl, #0
	ret
__endasm;
}

unsigned char passVarg(int p, ...) __dynamicc __naked
{
__asm
	ld	ix, #0xabcd
	ex	de, hl
	ld	hl, (_i)
	cp	a, a
	sbc	hl, de
	jr	nz, bad_varg
	pop	bc
	pop	hl
	push	hl
	push	bc
	cp	a, a
	sbc	hl, de
	jr	nz, bad_varg
	ld	hl, #1
	ret
bad_varg:
	ld	hl, #0
	ret
__endasm;
}

#else // Just do a very basic check for ports that don't have __dynamicc
unsigned char passChar(char p) __dynamicc
{
	return p == c;
}

unsigned char passInt(int p) __dynamicc
{
	return p == i;
}

unsigned char passLong(long p) __dynamicc
{
	return p == l;
}

unsigned char passPtr(char *p) __dynamicc
{
	return p == &c;
}

unsigned char passVarg(int p, ...) __dynamicc
{
	return p == i;
}

#endif

void testDynamicC(void)
{
	// Local variables likely placed on the stack.
	char lc = c;
	int li = i;
	long ll = l;
	ASSERT(passChar(c));
	ASSERT(passInt(i));
	ASSERT(passLong(l));
	ASSERT(passPtr(&c));
	ASSERT(passVarg(i));
	// Access the local variables. For testing that the handling of the frame pointer register ix works (for __dynamicc, ix is caller-saved instead of callee-saved).
	ASSERT(lc == c);
	ASSERT(li == i);
	ASSERT(ll == l);
}

