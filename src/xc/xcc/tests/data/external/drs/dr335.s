	.module xcc_output


	.area _CODE

	.globl _dr335
_dr335:
	; prologue: dr335 (locals=1)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-1
	add	hl, sp
	ld	sp, hl
	push	ix
	pop	hl
	ld	de, #-1
	add	hl, de
	dec	sp
	dec	sp
	ld	-3(ix), l
	ld	-2(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-1
	add	hl, de
	dec	sp
	dec	sp
	ld	-5(ix), l
	ld	-4(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-7(ix), l
	ld	-6(ix), h
	ld	l, -7(ix)
	ld	h, -6(ix)
	push	hl
	ld	hl, #1
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-9(ix), l
	ld	-8(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	dec	sp
	dec	sp
	ld	-11(ix), l
	ld	-10(ix), h
	push	ix
	pop	hl
	ld	de, #-1
	add	hl, de
	dec	sp
	dec	sp
	ld	-13(ix), l
	ld	-12(ix), h
	ld	l, -11(ix)
	ld	h, -10(ix)
	push	hl
	ld	hl, #1
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-15(ix), l
	ld	-14(ix), h
	ld	l, -13(ix)
	ld	h, -12(ix)
	ld	a, (hl)
	dec	sp
	dec	sp
	ld	-17(ix), a
	ld	l, -17(ix)
	ld	h, -16(ix)
	push	hl
	ld	hl, #-2
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -19(ix)
	ld	h, -18(ix)
	push	hl
	ld	l, -15(ix)
	ld	h, -14(ix)
	pop	de
	ld	a, l
	or	a, e
	ld	l, a
	ld	a, h
	or	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -13(ix)
	ld	h, -12(ix)
	push	hl
	ld	a, -21(ix)
	pop	hl
	ld	(hl), a
__dr335_end:
	; epilogue: dr335
	ld	sp, ix
	pop	ix
	ret
