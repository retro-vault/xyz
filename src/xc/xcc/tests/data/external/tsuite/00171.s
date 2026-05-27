	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 10, 0
__str_4:
	.db 98, 32, 105, 115, 32, 78, 85, 76, 76, 10, 0
__str_5:
	.db 98, 32, 105, 115, 32, 110, 111, 116, 32, 78, 85, 76, 76, 10, 0
__str_9:
	.db 99, 32, 105, 115, 32, 78, 85, 76, 76, 10, 0
__str_10:
	.db 99, 32, 105, 115, 32, 110, 111, 116, 32, 78, 85, 76, 76, 10, 0


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
	ld	hl, #42
	ld	-2(ix), l
	ld	-1(ix), h
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	jp	__xcc_L3
__xcc_L2:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
__xcc_L3:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L6:
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	jp	__xcc_L8
__xcc_L7:
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
__xcc_L8:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
