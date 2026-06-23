	.module xcc_output


	.area _CODE

	.globl _f1
_f1:
	; prologue: f1 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param f at 4(ix)
	; receive param p at 16(ix)
	; receive param n at 18(ix)
	push	ix
	pop	hl
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 16(ix)
	ld	h, 17(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #0
	jp	__f1_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, 16(ix)
	ld	h, 17(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, 18(ix)
	ld	d, 19(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	jp	__f1_end
__f1_end:
	; epilogue: f1
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #2
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _f1
	call	_f1
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #1
	push	hl
	ld	hl, #2
	push	hl
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _f1
	call	_f1
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
