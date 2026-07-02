	; fixed24_8_from_int.s
	; Convert signed int to 24.8 fixed.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_from_int
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_from_int
	.area	_CODE
	; inputs:  HL = signed int
	; outputs: DE:HL = HL << 8, sign-extended
_fixed24_8_from_int::
	ld	e,#0
	ld	d,l
	ld	l,h
	ld	h,#0
	bit	7,l
	ret	z
	dec	h
	ret
