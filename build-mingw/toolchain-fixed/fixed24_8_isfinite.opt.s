	; fixed24_8_isfinite.s
	; 24.8 fixed reserves the top encodings for +/-infinity and NaN.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_isfinite
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_isfinite
	.area	_CODE
_fixed24_8_isfinite::
	ld	a,h
	cp	#0x7f
	jr	z,.positive_top
	cp	#0x80
	jr	z,.negative_top
	ld	de,#1
	ret
.positive_top:
	ld	a,l
	cp	#0xff
	jr	nz,.true
	ld	a,d
	cp	#0xff
	jr	nz,.true
	ld	a,e
	cp	#0xff
	jr	z,.false
	cp	#0xfe
	jr	z,.false
	jr	.true
.negative_top:
	ld	a,l
	or	d
	or	e
	jr	z,.false
	ld	de,#1
	ret
.true:
	ld	de,#1
	ret
.false:
	ld	de,#0
	ret
