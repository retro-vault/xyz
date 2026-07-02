	; fixed8_8_fmax.s
	; Maximum for signed 8.8 fixed values.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_fmax
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_fmax
	.area	_CODE
	; inputs:  HL = x, DE = y
	; outputs: DE = max(x, y)
_fixed8_8_fmax::
	ld	a,h
	xor	d
	jp	m,.sign_diff
	ld	a,h
	cp	d
	jr	nz,.same_high
	ld	a,l
	cp	e
	jr	c,.use_y
	ex	de,hl
	ret
.same_high:
	jr	c,.use_y
	ex	de,hl
.use_y:
	ret
.sign_diff:
	bit	7,h
	jr	nz,.use_y
	ex	de,hl
	ret
