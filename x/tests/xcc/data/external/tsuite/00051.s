	.module xcc_output

	.area _DATA
	.globl _x
_x:
	.ds 2


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	jp	__xcc_L0
__xcc_L0:
	jp	__xcc_L1
__xcc_L1:
	ld	hl, (_x)
	push	hl
	ld	hl, #0
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
	jp	nz, __xcc_L2
	jp	__xcc_L3
__xcc_L2:
	jp	next
__xcc_L3:
	ld	hl, #1
	jp	__main_end
__xcc_L4:
	ld	hl, #1
	jp	__main_end
next:
	jp	__xcc_L5
__xcc_L5:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L6
	ld	hl, #1
	inc	hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	(_x), hl
foo:
	ld	hl, #1
	jp	__main_end
__xcc_L6:
	ld	hl, (_x)
	push	hl
	ld	hl, #0
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L7
	ld	hl, (_x)
	push	hl
	ld	hl, #1
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L8
	jp	__xcc_L9
__xcc_L7:
	ld	hl, (_x)
	jp	__main_end
__xcc_L8:
	ld	hl, #1
	jp	__main_end
__xcc_L9:
	ld	hl, #1
	jp	__main_end
__xcc_L10:
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
