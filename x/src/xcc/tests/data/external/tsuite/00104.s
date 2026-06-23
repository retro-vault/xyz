	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #4294967295
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
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #-1
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
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L5
__xcc_L5:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
