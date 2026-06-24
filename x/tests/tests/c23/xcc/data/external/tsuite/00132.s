	.module xcc_output

	.area _CONST
__str_0:
	.db 72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 10, 0
__str_5:
	.db 67, 111, 117, 110, 116, 32, 61, 32, 37, 100, 10, 0
__str_6:
	.db 83, 116, 114, 105, 110, 103, 32, 39, 104, 101, 108, 108, 111, 39, 44, 32, 39, 116, 104, 101, 114, 101, 39, 32, 105, 115, 32, 39, 37, 115, 39, 44, 32, 39, 37, 115, 39, 10, 0
__str_7:
	.db 104, 101, 108, 108, 111, 0
__str_8:
	.db 116, 104, 101, 114, 101, 0
__str_9:
	.db 67, 104, 97, 114, 97, 99, 116, 101, 114, 32, 39, 65, 39, 32, 105, 115, 32, 39, 37, 99, 39, 10, 0
__str_10:
	.db 67, 104, 97, 114, 97, 99, 116, 101, 114, 32, 39, 97, 39, 32, 105, 115, 32, 39, 37, 99, 39, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #5
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L1:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #5
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
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L2
	jp	__xcc_L4
__xcc_L2:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L3:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L1
__xcc_L4:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #65
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	hl, #97
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
