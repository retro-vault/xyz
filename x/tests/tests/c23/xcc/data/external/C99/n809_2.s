	.module xcc_output


	.area _CODE

	.globl _func
_func:
	; prologue: func (locals=16)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-16
	add	hl, sp
	ld	sp, hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __fsadd
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	push	de
	pop	hl
	ld	-18(ix), l
	ld	-17(ix), h
	.globl __fsadd
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	push	de
	pop	hl
	ld	-22(ix), l
	ld	-21(ix), h
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	.globl __fsadd
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	push	de
	pop	hl
	ld	-36(ix), l
	ld	-35(ix), h
	.globl __fsadd
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	push	de
	pop	hl
	ld	-40(ix), l
	ld	-39(ix), h
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	.globl __fsadd
	ld	hl, #0
	push	hl
	ld	hl, #1
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	push	de
	pop	hl
	ld	-54(ix), l
	ld	-53(ix), h
	.globl __fsadd
	ld	hl, #0
	push	hl
	ld	hl, #1
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	push	de
	pop	hl
	ld	-58(ix), l
	ld	-57(ix), h
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	l, -56(ix)
	ld	h, -55(ix)
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	hl, #1
	push	hl
	.globl _variadic
	call	_variadic
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
__func_end:
	; epilogue: func
	ld	sp, ix
	pop	ix
	ret
