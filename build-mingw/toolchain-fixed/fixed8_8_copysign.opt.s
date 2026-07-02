	; fixed8_8_copysign.s
	; Copy sign for signed 8.8 fixed values.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_copysign
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_copysign
	.globl	_fixed8_8_abs
	.globl	_fixed8_8_neg
	.area	_CODE
	; inputs:  HL = magnitude, DE = sign source
	; outputs: DE = copysign(magnitude, sign)
_fixed8_8_copysign::
	ld	a,d
	push	af
	call	_fixed8_8_abs
	pop	af
	bit	7,a
	ret	z
	ld	h, d
	ld	l, e
	jp	_fixed8_8_neg
