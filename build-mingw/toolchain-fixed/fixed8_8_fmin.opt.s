	; fixed8_8_fmin.s
	; Minimum for signed 8.8 fixed values.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_fmin
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_fmin
	.area	_CODE
	; inputs:  HL = x, DE = y
	; outputs: DE = min(x, y)
_fixed8_8_fmin::
	ld	a,h
	xor	d
	jp	m,.sign_diff
	ld	a,h
	cp	d
	jr	nz,.same_high
	ld	a,l
	cp	e
	ret	nc	; x >= y: y already in DE
	ex	de,hl
	ret
.same_high:
	ret	nc
	ex	de,hl
	ret
.sign_diff:
	bit	7,h
	jr	z,.x_positive
	ex	de,hl
.x_positive:
	ret
