	.module xcc_output


	.area _CODE

	.globl _one
_one:
	; prologue: one (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__one_end
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #0
	jp	__one_end
__one_end:
	; epilogue: one
	ld	sp, ix
	pop	ix
	ret
	.globl _two
_two:
	; prologue: two (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	; receive param b at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #1
	jp	__two_end
	jp	__xcc_L5
__xcc_L5:
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #1
	jp	__two_end
	jp	__xcc_L8
__xcc_L8:
	ld	hl, #0
	jp	__two_end
__two_end:
	; epilogue: two
	ld	sp, ix
	pop	ix
	ret
	.globl _three
_three:
	; prologue: three (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	; receive param b at 6(ix)
	; receive param c at 8(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L11
__xcc_L9:
	ld	hl, #1
	jp	__three_end
	jp	__xcc_L11
__xcc_L11:
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L14
__xcc_L12:
	ld	hl, #1
	jp	__three_end
	jp	__xcc_L14
__xcc_L14:
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L17
__xcc_L15:
	ld	hl, #1
	jp	__three_end
	jp	__xcc_L17
__xcc_L17:
	ld	hl, #0
	jp	__three_end
__three_end:
	; epilogue: three
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
	.globl _one
	call	_one
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L18
	jp	__xcc_L20
__xcc_L18:
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L20
__xcc_L20:
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	.globl _two
	call	_two
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L21
	jp	__xcc_L23
__xcc_L21:
	ld	hl, #3
	jp	__main_end
	jp	__xcc_L23
__xcc_L23:
	ld	hl, #3
	push	hl
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	.globl _three
	call	_three
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L24
	jp	__xcc_L26
__xcc_L24:
	ld	hl, #4
	jp	__main_end
	jp	__xcc_L26
__xcc_L26:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
