	; fixed8_8_add.s
	; Signed 8.8 add. Two's-complement overflow wraps.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_add
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_add
	.area	_CODE
	; inputs:  HL = a, DE = b
	; outputs: DE = a + b
_fixed8_8_add::
	add	hl,de
	ex	de,hl
	ret
