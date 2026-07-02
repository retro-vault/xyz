	; fixed8_8_to_16_16.s
	; Convert 8.8 fixed to 16.16 fixed.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_to_16_16
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_to_16_16
	.area	_CODE
	; inputs:  HL = fixed8_8
	; outputs: DE:HL = fixed16_16
_fixed8_8_to_16_16::
	ld	e,#0
	ld	d,l
	ld	l,h
	ld	h,#0
	bit	7,l
	ret	z
	dec	h
	ret
