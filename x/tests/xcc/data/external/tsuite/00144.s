	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #0
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	ld	-6(ix), l
	ld	-5(ix), h
__xcc_L2:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	jp	__xcc_L5
__xcc_L4:
	ld	hl, #0
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L5:
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L6:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	jp	__xcc_L8
__xcc_L7:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L8:
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L10
__xcc_L9:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	jp	__xcc_L11
__xcc_L10:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-18(ix), l
	ld	-17(ix), h
__xcc_L11:
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L13
__xcc_L12:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	jp	__xcc_L14
__xcc_L13:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-22(ix), l
	ld	-21(ix), h
__xcc_L14:
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L16
__xcc_L15:
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	jp	__xcc_L17
__xcc_L16:
	ld	hl, #0
	ld	-24(ix), l
	ld	-23(ix), h
__xcc_L17:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L18
	jp	__xcc_L19
__xcc_L18:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	jp	__xcc_L20
__xcc_L19:
	ld	hl, #0
	ld	-26(ix), l
	ld	-25(ix), h
__xcc_L20:
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L21
	jp	__xcc_L22
__xcc_L21:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	jp	__xcc_L23
__xcc_L22:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-28(ix), l
	ld	-27(ix), h
__xcc_L23:
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
