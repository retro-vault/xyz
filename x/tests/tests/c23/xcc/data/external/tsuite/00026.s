	.module xcc_output

	.area _CONST
__str_0:
	.db 104, 101, 108, 108, 111, 0


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
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -6(ix)
	ld	d, -5(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	de, #104
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
