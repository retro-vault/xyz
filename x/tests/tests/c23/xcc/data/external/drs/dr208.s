	.module xcc_output


	.area _CODE

	.globl _dr208
_dr208:
	; prologue: dr208 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #0
	push	hl
	.globl _dr208_init
	call	_dr208_init
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	e, -8(ix)
	ld	d, -7(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #1
	push	hl
	.globl _dr208_init
	call	_dr208_init
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	e, -10(ix)
	ld	d, -9(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #2
	push	hl
	.globl _dr208_init
	call	_dr208_init
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	e, -14(ix)
	ld	d, -13(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__dr208_end:
	; epilogue: dr208
	ld	sp, ix
	pop	ix
	ret
