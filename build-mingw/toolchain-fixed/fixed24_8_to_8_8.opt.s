	; fixed24_8_to_8_8.s
	; Convert 24.8 fixed to 8.8 fixed by taking the low raw word.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_to_8_8
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_to_8_8
	.area	_CODE
	; inputs:  DE:HL = fixed24_8
	; outputs: DE = fixed8_8
_fixed24_8_to_8_8::
	ret
