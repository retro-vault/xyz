	; fixed16_16_fpclassify.s
	; C fpclassify-style classification for signed 16.16 fixed float mode.
	; The top encodings are reserved for +/-infinity and NaN.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_fpclassify
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_fpclassify
	.area	_CODE
	; inputs:  DE:HL = fixed16_16
	; outputs: DE = FP_NAN (0), FP_INFINITE (1), FP_ZERO (2),
	; or FP_NORMAL (4)
_fixed16_16_fpclassify::
	ld	a,h
	cp	#0x7f
	jr	z,.positive_top
	cp	#0x80
	jr	z,.negative_top
	ld	a,h
	or	l
	or	d
	or	e
	ld	de,#2
	ret	z
	ld	de,#4
	ret
.positive_top:
	ld	a,l
	cp	#0xff
	jr	nz,.normal
	ld	a,d
	cp	#0xff
	jr	nz,.normal
	ld	a,e
	cp	#0xff
	jr	z,.infinite
	cp	#0xfe
	jr	z,.nan
.normal:
	ld	de,#4
	ret
.negative_top:
	ld	a,l
	or	d
	or	e
	jr	z,.infinite
	ld	de,#4
	ret
.nan:
	ld	de,#0
	ret
.infinite:
	ld	de,#1
	ret
