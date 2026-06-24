	.module xcc_output

	.area _CONST
__str_4:
	.db 49, 10, 0
__str_5:
	.db 50, 10, 0
__str_6:
	.db 51, 10, 0
__str_7:
	.db 111, 117, 116, 10, 0


	.area _CODE

	.globl _fred
_fred:
	; prologue: fred (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L2
	jp	__xcc_L3
__xcc_L0:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__fred_end
__xcc_L1:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	jp	__xcc_L3
__xcc_L2:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	jp	__fred_end
__xcc_L3:
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
__fred_end:
	; epilogue: fred
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	push	hl
	.globl _fred
	call	_fred
	pop	bc
	ld	hl, #2
	push	hl
	.globl _fred
	call	_fred
	pop	bc
	ld	hl, #3
	push	hl
	.globl _fred
	call	_fred
	pop	bc
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
