	; fixed16_16_fdim.s
	; Positive difference for signed 16.16 fixed values.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_fdim
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_fdim
	.area	_CODE
	; inputs:  DE:HL = x, stack = y
	; outputs: DE:HL = x > y ? x - y : 0
_fixed16_16_fdim::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	a,h
	xor	7(ix)
	jp	m,.sign_diff
	ld	a,h
	cp	7(ix)
	jr	nz,.byte
	ld	a,l
	cp	6(ix)
	jr	nz,.byte
	ld	a,d
	cp	5(ix)
	jr	nz,.byte
	ld	a,e
	cp	4(ix)
	jr	z,.zero
	jr	c,.zero
	jr	.subtract
.byte:
	jr	c,.zero
	jr	.subtract
.sign_diff:
	bit	7,h
	jr	nz,.zero
.subtract:
	or	a
	ld	a,e
	sbc	a,4(ix)
	ld	e,a
	ld	a,d
	sbc	a,5(ix)
	ld	d,a
	ld	a,l
	sbc	a,6(ix)
	ld	l,a
	ld	a,h
	sbc	a,7(ix)
	ld	h,a
	pop	ix
	ret
.zero:
	ld	de,#0
	ld	hl,#0
	pop	ix
	ret
