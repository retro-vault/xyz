	; fixed16_16_from_int.s
	; Convert signed int to 16.16 fixed.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_from_int
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_from_int
	.area	_CODE
	; inputs:  HL = signed int
	; outputs: DE:HL = HL << 16
_fixed16_16_from_int::
	ld	de, #0
	ret
