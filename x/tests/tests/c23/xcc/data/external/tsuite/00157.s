	.module xcc_output

	.area _CONST
__str_8:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=22)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-22
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L0:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #10
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89383
	jp	m, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	.globl __mul16
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -28(ix)
	ld	h, -27(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	e, -30(ix)
	ld	d, -29(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	e, -26(ix)
	ld	d, -25(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L2:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L4:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #10
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	jp	__xcc_L7
__xcc_L5:
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	e, -40(ix)
	ld	d, -39(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
__xcc_L6:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L4
__xcc_L7:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
