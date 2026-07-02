	; fixed8_8_fpclassify.s
	; C fpclassify-style classification for signed 8.8 fixed float mode.
	; The top encodings are reserved for +/-infinity and NaN.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_fpclassify
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_fpclassify
	.area	_CODE
	; inputs:  HL = fixed8_8
	; outputs: DE = FP_NAN (0), FP_INFINITE (1), FP_ZERO (2),
	; or FP_NORMAL (4)
_fixed8_8_fpclassify::
	ld	a,h
	cp	#0x7f
	jr	z,.positive_top
	cp	#0x80
	jr	z,.negative_top
	ld	a,h
	or	l
	ld	de,#2
	ret	z
	ld	de,#4
	ret
.positive_top:
	ld	a,l
	cp	#0xff
	jr	z,.infinite
	cp	#0xfe
	jr	z,.nan
	ld	de,#4
	ret
.negative_top:
	ld	a,l
	or	a
	jr	z,.infinite
	ld	de,#4
	ret
.nan:
	ld	de,#0
	ret
.infinite:
	ld	de,#1
	ret
