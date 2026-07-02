	; fixed24_8_floor.s
	; Floor signed 24.8 by clearing the fractional byte.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_floor
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_floor
	.area	_CODE
	; inputs:  DE:HL = fixed24_8
	; outputs: DE:HL = floor(x)
_fixed24_8_floor::
	ld	e,#0
	ret
