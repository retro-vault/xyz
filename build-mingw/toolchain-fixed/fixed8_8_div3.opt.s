	; fixed8_8_div3.s
	; Signed 8.8 fixed-point divide by exact integer 3.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_div3
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_div3
	.area	_CODE
	; inputs:  HL = a
	; outputs: DE = a / 3, truncated toward zero
_fixed8_8_div3::
	bit	7,h
	jr	z,.unsigned
	call	.neg_hl
	call	.u16_div3
	call	.neg_hl
	ex	de,hl
	ret
.unsigned:
	call	.u16_div3
	ex	de,hl
	ret
.u16_div3:
	ld	bc, #4096
.u16_div3_loop:
	sla	l
	rl	h
	ld	a,c
	rla
	ld	c,a
	cp	#3
	jr	c,.u16_div3_next
	sub	a,#3
	ld	c,a
	set	0,l
.u16_div3_next:
	djnz	.u16_div3_loop
	ret
.neg_hl:
	xor	a
	sub	a,l
	ld	l,a
	ld	a,#0
	sbc	a,h
	ld	h,a
	ret
