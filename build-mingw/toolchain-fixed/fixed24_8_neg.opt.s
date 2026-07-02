	; fixed24_8_neg.s
	; Signed 24.8 negation. Two's-complement overflow wraps.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_neg
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_neg
	.area	_CODE
	; inputs:  DE:HL = a
	; outputs: DE:HL = -a
_fixed24_8_neg::
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	ld	a,#0
	sbc	a,l
	ld	l,a
	ld	a,#0
	sbc	a,h
	ld	h,a
	ret
