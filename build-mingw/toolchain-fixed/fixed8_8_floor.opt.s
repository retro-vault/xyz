	; fixed8_8_floor.s
	; Floor signed 8.8 by clearing the fractional byte. For two's-complement
	; fixed-point, this is naturally round toward -infinity.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_floor
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_floor
	.area	_CODE
	; inputs:  HL = fixed8_8
	; outputs: DE = floor(x)
_fixed8_8_floor::
	ld	l,#0
	ex	de,hl
	ret
