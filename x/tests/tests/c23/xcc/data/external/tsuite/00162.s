	.module xcc_output


	.area _CODE

	.globl _fooc
_fooc:
	; prologue: fooc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #3
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	de, #42
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__fooc_end:
	; epilogue: fooc
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
