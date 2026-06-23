	.module xcc_output


	.area _CODE

	.globl _dr206
_dr206:
	; prologue: dr206 (locals=8)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _dr206_unprototyped
	call	_dr206_unprototyped
	pop	bc
	pop	bc
	pop	bc
	pop	bc
__dr206_end:
	; epilogue: dr206
	ld	sp, ix
	pop	ix
	ret
