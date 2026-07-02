	; fixed16_16_isnan.s
	; 16.16 fixed reserves 0x7ffffffe for NaN.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_isnan
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_isnan
	.area	_CODE
_fixed16_16_isnan::
	ld	a,h
	cp	#0x7f
	jr	nz,.false
	ld	a,l
	cp	#0xff
	jr	nz,.false
	ld	a,d
	cp	#0xff
	jr	nz,.false
	ld	a,e
	cp	#0xfe
	jr	z,.true
.false:
	ld	de,#0
	ret
.true:
	ld	de,#1
	ret
