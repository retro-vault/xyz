	.module xcc_output

	.area _CONST
__str_3:
	.db 69, 114, 114, 111, 114, 58, 32, 48, 32, 60, 32, 45, 50, 49, 52, 55, 52, 56, 51, 54, 52, 56, 10, 0
__str_7:
	.db 69, 114, 114, 111, 114, 58, 32, 50, 49, 52, 55, 52, 56, 51, 54, 52, 55, 32, 60, 32, 48, 10, 0
__str_8:
	.db 108, 111, 110, 103, 32, 108, 111, 110, 103, 32, 99, 111, 110, 115, 116, 97, 110, 116, 32, 116, 101, 115, 116, 32, 111, 107, 46, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=8)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #2147483648
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #2147483647
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
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
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L5
__xcc_L4:
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L6
__xcc_L5:
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
__xcc_L6:
__xcc_L2:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
