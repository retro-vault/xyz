	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -6(ix)
	ld	d, -5(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
