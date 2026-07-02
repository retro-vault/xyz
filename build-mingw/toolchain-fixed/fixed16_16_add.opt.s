	; fixed16_16_add.s
	; Signed 16.16 add. Two's-complement overflow wraps.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_add
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_add
	.area	_CODE
	; inputs:  DE:HL = a, 4(ix)..7(ix) = b
	; outputs: DE:HL = a + b
_fixed16_16_add::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	a,e
	add	a,4(ix)
	ld	e,a
	ld	a,d
	adc	a,5(ix)
	ld	d,a
	ld	a,l
	adc	a,6(ix)
	ld	l,a
	ld	a,h
	adc	a,7(ix)
	ld	h,a
	pop	ix
	ret
