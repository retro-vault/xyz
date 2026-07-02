	; fixed16_16_trunc.s
	; Round signed 16.16 toward zero.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_trunc
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_trunc
	.globl	_fixed16_16_to_int
	.globl	_fixed16_16_from_int
	.area	_CODE
	; inputs:  DE:HL = fixed16_16
	; outputs: DE:HL = trunc(x)
_fixed16_16_trunc::
	call	_fixed16_16_to_int
	ld	h, d
	ld	l, e
	jp	_fixed16_16_from_int
