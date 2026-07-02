	; fixed8_8_to_int.s
	; Convert 8.8 fixed to signed int by truncating toward zero.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_to_int
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_to_int
	.area	_CODE
	; inputs:  HL = fixed8_8
	; outputs: DE = signed int
_fixed8_8_to_int::
	bit	7,h
	jr	z,.positive
	call	.neg_hl
	ld	e,h
	ld	d,#0
	jr	.neg_de
.positive:
	ld	e,h
	ld	d,#0
	ret
.neg_hl:
	xor	a
	sub	a,l
	ld	l,a
	ld	a,#0
	sbc	a,h
	ld	h,a
	ret
.neg_de:
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	ret
