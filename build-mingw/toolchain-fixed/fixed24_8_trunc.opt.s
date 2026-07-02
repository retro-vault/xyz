	; fixed24_8_trunc.s
	; Round signed 24.8 toward zero.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_trunc
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_trunc
	.globl	_fixed24_8_to_int
	.globl	_fixed24_8_from_int
	.area	_CODE
	; inputs:  DE:HL = fixed24_8
	; outputs: DE:HL = trunc(x)
_fixed24_8_trunc::
	call	_fixed24_8_to_int
	ld	h, d
	ld	l, e
	jp	_fixed24_8_from_int
