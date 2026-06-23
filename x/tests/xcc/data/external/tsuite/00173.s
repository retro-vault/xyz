	.module xcc_output

	.area _CONST
__str_0:
	.db 104, 101, 108, 108, 111, 0
__str_1:
	.db 37, 115, 10, 0
__str_6:
	.db 37, 99, 58, 32, 37, 100, 10, 0
__str_10:
	.db 99, 111, 112, 105, 101, 100, 32, 115, 116, 114, 105, 110, 103, 32, 105, 115, 32, 37, 115, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=23)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-23
	add	hl, sp
	ld	sp, hl
	ld	hl, #97
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-3(ix), l
	ld	-2(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-25(ix), l
	ld	-24(ix), h
	ld	l, -25(ix)
	ld	h, -24(ix)
	ld	-5(ix), l
	ld	-4(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-27(ix), l
	ld	-26(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	push	hl
	ld	l, -27(ix)
	ld	h, -26(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-29(ix), l
	ld	-28(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-31(ix), l
	ld	-30(ix), h
	ld	l, -31(ix)
	ld	h, -30(ix)
	ld	-7(ix), l
	ld	-6(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	-9(ix), l
	ld	-8(ix), h
__xcc_L2:
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-33(ix), l
	ld	-32(ix), h
	ld	l, -33(ix)
	ld	h, -32(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-35(ix), l
	ld	-34(ix), h
	ld	l, -35(ix)
	ld	h, -34(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-37(ix), l
	ld	-36(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-39(ix), l
	ld	-38(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-41(ix), l
	ld	-40(ix), h
	ld	l, -41(ix)
	ld	h, -40(ix)
	push	hl
	ld	l, -39(ix)
	ld	h, -38(ix)
	push	hl
	ld	l, -37(ix)
	ld	h, -36(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-43(ix), l
	ld	-42(ix), h
__xcc_L4:
	ld	l, -9(ix)
	ld	h, -8(ix)
	dec	sp
	dec	sp
	ld	-45(ix), l
	ld	-44(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	inc	hl
	ld	-9(ix), l
	ld	-8(ix), h
	jp	__xcc_L2
__xcc_L5:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-47(ix), l
	ld	-46(ix), h
	ld	l, -19(ix)
	ld	h, -18(ix)
	ld	e, -47(ix)
	ld	d, -46(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-49(ix), l
	ld	-48(ix), h
	ld	l, -49(ix)
	ld	h, -48(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-51(ix), l
	ld	-50(ix), h
	push	ix
	pop	hl
	ld	de, #-51
	add	hl, de
	dec	sp
	dec	sp
	ld	-53(ix), l
	ld	-52(ix), h
	ld	l, -53(ix)
	ld	h, -52(ix)
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	-23(ix), l
	ld	-22(ix), h
__xcc_L7:
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-55(ix), l
	ld	-54(ix), h
	ld	l, -55(ix)
	ld	h, -54(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-57(ix), l
	ld	-56(ix), h
	ld	l, -57(ix)
	ld	h, -56(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L8
	jp	__xcc_L9
__xcc_L8:
	ld	l, -23(ix)
	ld	h, -22(ix)
	dec	sp
	dec	sp
	ld	-59(ix), l
	ld	-58(ix), h
	ld	l, -23(ix)
	ld	h, -22(ix)
	inc	hl
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -59(ix)
	ld	h, -58(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-61(ix), l
	ld	-60(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	dec	sp
	dec	sp
	ld	-63(ix), l
	ld	-62(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	inc	hl
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -63(ix)
	ld	h, -62(ix)
	push	hl
	ld	e, -61(ix)
	ld	d, -60(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L7
__xcc_L9:
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-65(ix), l
	ld	-64(ix), h
	ld	l, -19(ix)
	ld	h, -18(ix)
	push	hl
	ld	l, -65(ix)
	ld	h, -64(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-67(ix), l
	ld	-66(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
