	.module xcc_output

	.area _CONST
__str_3:
	.db 37, 100, 0
__str_7:
	.db 37, 99, 0
__str_8:
	.db 101, 0
__str_9:
	.db 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=3)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-3
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L0:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #2
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-5(ix), l
	ld	-4(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-7(ix), l
	ld	-6(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-9(ix), l
	ld	-8(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	ld	l, -7(ix)
	ld	h, -6(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-11(ix), l
	ld	-10(ix), h
	jp	__xcc_L2
	ld	hl, #65
	ld	-3(ix), l
	ld	-2(ix), h
__xcc_L4:
	ld	l, -3(ix)
	ld	h, -2(ix)
	push	hl
	ld	hl, #67
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
	ld	-13(ix), l
	ld	-12(ix), h
	ld	l, -13(ix)
	ld	h, -12(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	jp	__xcc_L6
__xcc_L5:
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-15(ix), l
	ld	-14(ix), h
	ld	a, -3(ix)
	dec	sp
	dec	sp
	ld	-17(ix), a
	ld	l, -3(ix)
	ld	h, -2(ix)
	inc	hl
	ld	-3(ix), l
	ld	-2(ix), h
	ld	a, -17(ix)
	ld	l, a
	ld	h, #0
	push	hl
	ld	l, -15(ix)
	ld	h, -14(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-19(ix), l
	ld	-18(ix), h
	jp	__xcc_L4
__xcc_L6:
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-23(ix), l
	ld	-22(ix), h
	jp	__xcc_L0
__xcc_L2:
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-25(ix), l
	ld	-24(ix), h
	ld	l, -25(ix)
	ld	h, -24(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-27(ix), l
	ld	-26(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
