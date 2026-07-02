	; fixed24_8_cmp.s
	; Signed 24.8 compare.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_cmp
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_cmp
	.area	_CODE
	; inputs:  DE:HL = a, 4(ix)..7(ix) = b
	; outputs: DE = -1 if a < b, 0 if a == b, +1 if a > b
_fixed24_8_cmp::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	a,h
	xor	7(ix)
	jp	m,.sign_diff
	ld	a,h
	cp	7(ix)
	jr	nz,.byte3
	ld	a,l
	cp	6(ix)
	jr	nz,.byte2
	ld	a,d
	cp	5(ix)
	jr	nz,.byte1
	ld	a,e
	cp	4(ix)
	jr	z,.eq
	jr	c,.lt
	jr	.gt
.byte3:
.byte2:
.byte1:
	jr	c,.lt
	jr	.gt
.sign_diff:
	bit	7,h
	jr	nz,.lt
	jr	.gt
.eq:
	ld	de,#0
	pop	ix
	ret
.lt:
	ld	de,#0xffff
	pop	ix
	ret
.gt:
	ld	de,#1
	pop	ix
	ret
