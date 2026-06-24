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
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	pop	de
	adc	hl, de
	ld	-8(ix), l
	ld	-7(ix), h
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	pop	de
	adc	hl, de
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
