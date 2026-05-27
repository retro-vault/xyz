	.module xcc_output

	.area _CONST
__str_4:
	.db 37, 100, 10, 0


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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L2:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
