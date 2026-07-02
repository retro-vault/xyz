	; fixed16_16_div4.s
	; Signed 16.16 fixed-point divide by exact integer 4.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_div4
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_div4
	.area	_CODE
	; inputs:  DE:HL = a
	; outputs: DE:HL = a / 4, truncated toward zero
_fixed16_16_div4::
	bit	7,h
	jr	z,.shift
	call	.neg_dehl
	call	.shr_dehl_1
	call	.shr_dehl_1
	jr	.neg_dehl
.shift:
	call	.shr_dehl_1
	jr	.shr_dehl_1
.shr_dehl_1:
	srl	h
	rr	l
	rr	d
	rr	e
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
