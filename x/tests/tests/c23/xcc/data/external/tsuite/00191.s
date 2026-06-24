	.module xcc_output

	.area _CONST
__str_4:
	.db 105, 116, 39, 115, 32, 97, 108, 108, 32, 103, 111, 111, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L2:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
