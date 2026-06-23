	.module xcc_output


	.area _CODE

	.globl _test
_test:
	; prologue: test (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
	push	ix
	pop	hl
	ld	de, #-8
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	e, -2(ix)
	ld	d, -1(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
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
	ld	e, -4(ix)
	ld	d, -3(ix)
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
	ld	e, -2(ix)
	ld	d, -1(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__test_end:
	; epilogue: test
	ld	sp, ix
	pop	ix
	ret
