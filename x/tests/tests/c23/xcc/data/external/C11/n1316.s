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
	; receive param in_f at 4(ix)
	; receive param in_vp at 8(ix)
	ld	l, 8(ix)
	ld	h, 9(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	.globl _call_ptr
	call	_call_ptr
	pop	bc
	pop	bc
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _call_float
	call	_call_float
	pop	bc
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-4(ix), l
	ld	-3(ix), h
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
	push	hl
	ld	e, -4(ix)
	ld	d, -3(ix)
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
	ld	e, -6(ix)
	ld	d, -5(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__test_end:
	; epilogue: test
	ld	sp, ix
	pop	ix
	ret
