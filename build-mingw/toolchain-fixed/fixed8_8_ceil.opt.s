	; fixed8_8_ceil.s
	; Ceiling via -floor(-x).
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_ceil
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_ceil
	.globl	_fixed8_8_neg
	.globl	_fixed8_8_floor
	.area	_CODE
	; inputs:  HL = fixed8_8
	; outputs: DE = ceil(x)
_fixed8_8_ceil::
	call	_fixed8_8_neg
	ld	h, d
	ld	l, e
	call	_fixed8_8_floor
	ld	h, d
	ld	l, e
	jp	_fixed8_8_neg
