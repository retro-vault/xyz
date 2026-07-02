	; fixed16_16_to_int.s
	; Convert 16.16 fixed to signed int by truncating toward zero.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_to_int
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_to_int
	.area	_CODE
	; inputs:  DE:HL = fixed16_16
	; outputs: DE = signed int
_fixed16_16_to_int::
	bit	7,h
	jr	z,.positive
	call	.neg_dehl
	ex	de,hl
	jr	.neg_de
.positive:
	ex	de,hl
	ret
.neg_dehl:
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	ld	a,#0
	sbc	a,l
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
