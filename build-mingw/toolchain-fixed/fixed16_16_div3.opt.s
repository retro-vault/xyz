	; fixed16_16_div3.s
	; Signed 16.16 fixed-point divide by exact integer 3.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_div3
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_div3
	.area	_CODE
	; inputs:  DE:HL = a
	; outputs: DE:HL = a / 3, truncated toward zero
_fixed16_16_div3::
	bit	7,h
	jr	z,.unsigned
	call	.neg_dehl
	call	.u32_div3
	jr	.neg_dehl
.unsigned:
	jr	.u32_div3
.u32_div3:
	ld	bc, #8192
.u32_div3_loop:
	sla	e
	rl	d
	rl	l
	rl	h
	ld	a,c
	rla
	ld	c,a
	cp	#3
	jr	c,.u32_div3_next
	sub	a,#3
	ld	c,a
	set	0,e
.u32_div3_next:
	djnz	.u32_div3_loop
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
