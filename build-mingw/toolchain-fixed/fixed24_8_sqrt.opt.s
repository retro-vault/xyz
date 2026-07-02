	; fixed24_8_sqrt.s
	; Square root for non-negative signed 24.8 using Newton iteration:
	; g = (g + x / g) / 2
	; Negative inputs return zero (fixed mode has no NaN).
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_sqrt
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_sqrt
	.globl	_fixed24_8_div
	.globl	_fixed24_8_add
	.globl	_fixed24_8_div2
	.area	_CODE
SQ24_X	.equ	-4
SQ24_G	.equ	-8
SQ24_COUNT	.equ	-9
	; inputs:  DE:HL = fixed24_8
	; outputs: DE:HL = sqrt(x)
_fixed24_8_sqrt::
	push	ix
	ld	ix,#0
	add	ix,sp
	bit	7,h
	jp	nz,.zero
	ld	a,h
	or	l
	or	d
	or	e
	jp	z,.zero
	ld	b,h
	ld	c,l
	ld	hl,#-9
	add	hl,sp
	ld	sp,hl
	ld	SQ24_X(ix),e
	ld	SQ24_X+1(ix),d
	ld	SQ24_X+2(ix),c
	ld	SQ24_X+3(ix),b
	; Guess is x for x >= 1.0, otherwise 1.0.
	ld	a,b
	or	c
	or	d
	jr	nz,.guess_x
	ld	de,#0x0100
	ld	hl,#0
	jr	.store_guess
.guess_x:
	ld	e,SQ24_X(ix)
	ld	d,SQ24_X+1(ix)
	ld	l,SQ24_X+2(ix)
	ld	h,SQ24_X+3(ix)
.store_guess:
	ld	SQ24_G(ix),e
	ld	SQ24_G+1(ix),d
	ld	SQ24_G+2(ix),l
	ld	SQ24_G+3(ix),h
	ld	SQ24_COUNT(ix),#8
.loop:
	; x / g
	ld	l,SQ24_G+2(ix)
	ld	h,SQ24_G+3(ix)
	push	hl
	ld	l,SQ24_G(ix)
	ld	h,SQ24_G+1(ix)
	push	hl
	ld	e,SQ24_X(ix)
	ld	d,SQ24_X+1(ix)
	ld	l,SQ24_X+2(ix)
	ld	h,SQ24_X+3(ix)
	call	_fixed24_8_div
	pop	bc
	pop	bc
	; (g + x/g) / 2
	ld	l,SQ24_G+2(ix)
	ld	h,SQ24_G+3(ix)
	push	hl
	ld	l,SQ24_G(ix)
	ld	h,SQ24_G+1(ix)
	push	hl
	call	_fixed24_8_add
	pop	bc
	pop	bc
	call	_fixed24_8_div2
	ld	SQ24_G(ix),e
	ld	SQ24_G+1(ix),d
	ld	SQ24_G+2(ix),l
	ld	SQ24_G+3(ix),h
	ld	a,SQ24_COUNT(ix)
	dec	a
	ld	SQ24_COUNT(ix),a
	jr	nz,.loop
	ld	sp,ix
	pop	ix
	ret
.zero:
	ld	de,#0
	ld	hl,#0
	pop	ix
	ret
