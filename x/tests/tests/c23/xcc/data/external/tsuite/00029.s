	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #3
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #2
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
