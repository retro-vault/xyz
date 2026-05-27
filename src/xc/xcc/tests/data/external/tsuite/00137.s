	.module xcc_output

	.area _CONST
__str_0:
	.db 104, 101, 108, 108, 111, 32, 105, 115, 32, 98, 101, 116, 116, 101, 114, 32, 116, 104, 97, 110, 32, 98, 121, 101, 0


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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	hl, #104
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__xcc_L3
__xcc_L2:
	ld	hl, #1
	ld	-10(ix), l
	ld	-9(ix), h
__xcc_L3:
	ld	l, -10(ix)
	ld	h, -9(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
