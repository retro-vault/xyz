	; fixed16_16_div.s
	; Signed 16.16 fixed-point divide:
	; result = (a << 16) / b
	; Public ABI is int32 raw fixed (DE low16, HL high16).  The widened
	; numerator and restoring divider state are local byte scratch only.
	; Division by zero returns zero.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_div
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_div
	.area	_CODE
	; locals:
	; -1         sign flag
	; -9..-2     quotient/dividend64, low..high
	; -17..-10   divisor64, low..high
	; -25..-18   remainder64, low..high
	; inputs:  DE:HL = a, 4(ix)..7(ix) = b
	; outputs: DE:HL = signed ((a << 16) / b)
_fixed16_16_div::
	push	ix
	ld	ix,#0
	add	ix,sp
	; Exact positive integer divisors 2, 3, 4, and 8 are common in fixed
	; code and do not need the full widened restoring divider.
	ld	a,4(ix)
	or	5(ix)
	jr	nz,.check_zero
	ld	a,7(ix)
	or	a
	jr	nz,.check_zero
	ld	a,6(ix)
	cp	#2
	jp	z,.div_by_2
	cp	#3
	jp	z,.div_by_3
	cp	#4
	jp	z,.div_by_4
	cp	#8
	jp	z,.div_by_8
.check_zero:
	ld	a,4(ix)
	or	5(ix)
	or	6(ix)
	or	7(ix)
	jr	nz,.nonzero
	ld	de,#0
	ld	hl,#0
	pop	ix
	ret
.nonzero:
	ld	b,h
	ld	c,l
	ld	hl,#-25
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	xor	a
	ld	-1(ix),a
	bit	7,h
	jr	z,.a_abs_done
	ld	-1(ix),#1
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
.a_abs_done:
	; q = abs(a) << 32.  This pre-aligns the highest possible
	; (abs(a) << 16) numerator bit with the 64-bit divider's top bit,
	; letting the restoring divider run 48 useful iterations instead
	; of spending 16 cycles through leading zeros.
	xor	a
	ld	-9(ix),a
	ld	-8(ix),a
	ld	-7(ix),a
	ld	-6(ix),a
	ld	-5(ix),e
	ld	-4(ix),d
	ld	-3(ix),l
	ld	-2(ix),h
	ld	a,4(ix)
	ld	-17(ix),a
	ld	a,5(ix)
	ld	-16(ix),a
	ld	a,6(ix)
	ld	-15(ix),a
	ld	a,7(ix)
	ld	-14(ix),a
	xor	a
	ld	-13(ix),a
	ld	-12(ix),a
	ld	-11(ix),a
	ld	-10(ix),a
	bit	7,-14(ix)
	jr	z,.b_abs_done
	ld	a,-1(ix)
	xor	#1
	ld	-1(ix),a
	xor	a
	sub	a,-17(ix)
	ld	-17(ix),a
	ld	a,#0
	sbc	a,-16(ix)
	ld	-16(ix),a
	ld	a,#0
	sbc	a,-15(ix)
	ld	-15(ix),a
	ld	a,#0
	sbc	a,-14(ix)
	ld	-14(ix),a
.b_abs_done:
	xor	a
	ld	-25(ix),a
	ld	-24(ix),a
	ld	-23(ix),a
	ld	-22(ix),a
	ld	-21(ix),a
	ld	-20(ix),a
	ld	-19(ix),a
	ld	-18(ix),a
	ld	b,#48
.div_loop:
	sla	-9(ix)
	rl	-8(ix)
	rl	-7(ix)
	rl	-6(ix)
	rl	-5(ix)
	rl	-4(ix)
	rl	-3(ix)
	rl	-2(ix)
	rl	-25(ix)
	rl	-24(ix)
	rl	-23(ix)
	rl	-22(ix)
	rl	-21(ix)
	rl	-20(ix)
	rl	-19(ix)
	rl	-18(ix)
	or	a
	ld	a,-25(ix)
	sbc	a,-17(ix)
	ld	-25(ix),a
	ld	a,-24(ix)
	sbc	a,-16(ix)
	ld	-24(ix),a
	ld	a,-23(ix)
	sbc	a,-15(ix)
	ld	-23(ix),a
	ld	a,-22(ix)
	sbc	a,-14(ix)
	ld	-22(ix),a
	ld	a,-21(ix)
	sbc	a,-13(ix)
	ld	-21(ix),a
	ld	a,-20(ix)
	sbc	a,-12(ix)
	ld	-20(ix),a
	ld	a,-19(ix)
	sbc	a,-11(ix)
	ld	-19(ix),a
	ld	a,-18(ix)
	sbc	a,-10(ix)
	ld	-18(ix),a
	jr	nc,.keep_sub
	or	a
	ld	a,-25(ix)
	adc	a,-17(ix)
	ld	-25(ix),a
	ld	a,-24(ix)
	adc	a,-16(ix)
	ld	-24(ix),a
	ld	a,-23(ix)
	adc	a,-15(ix)
	ld	-23(ix),a
	ld	a,-22(ix)
	adc	a,-14(ix)
	ld	-22(ix),a
	ld	a,-21(ix)
	adc	a,-13(ix)
	ld	-21(ix),a
	ld	a,-20(ix)
	adc	a,-12(ix)
	ld	-20(ix),a
	ld	a,-19(ix)
	adc	a,-11(ix)
	ld	-19(ix),a
	ld	a,-18(ix)
	adc	a,-10(ix)
	ld	-18(ix),a
	jr	.next_bit
.keep_sub:
	set	0,-9(ix)
.next_bit:
	dec	b
	jp	nz,.div_loop
	ld	a,-1(ix)
	or	a
	jp	nz,.ret_neg
	ld	e,-9(ix)
	ld	d,-8(ix)
	ld	l,-7(ix)
	ld	h,-6(ix)
	ld	sp,ix
	pop	ix
	ret
.div_by_2:
	bit	7,h
	jr	z,.div_by_2_shift
	call	.neg_dehl
	call	.shr_dehl_1
	call	.neg_dehl
	pop	ix
	ret
.div_by_2_shift:
	call	.shr_dehl_1
	pop	ix
	ret
.div_by_4:
	bit	7,h
	jr	z,.div_by_4_shift
	call	.neg_dehl
	call	.shr_dehl_1
	call	.shr_dehl_1
	call	.neg_dehl
	pop	ix
	ret
.div_by_4_shift:
	call	.shr_dehl_1
	call	.shr_dehl_1
	pop	ix
	ret
.div_by_8:
	bit	7,h
	jr	z,.div_by_8_shift
	call	.neg_dehl
	call	.shr_dehl_1
	call	.shr_dehl_1
	call	.shr_dehl_1
	call	.neg_dehl
	pop	ix
	ret
.div_by_8_shift:
	call	.shr_dehl_1
	call	.shr_dehl_1
	call	.shr_dehl_1
	pop	ix
	ret
.div_by_3:
	bit	7,h
	jr	z,.div_by_3_unsigned
	call	.neg_dehl
	call	.u32_div3
	call	.neg_dehl
	pop	ix
	ret
.div_by_3_unsigned:
	call	.u32_div3
	pop	ix
	ret
.shr_dehl_1:
	srl	h
	rr	l
	rr	d
	rr	e
	ret
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
.ret_neg:
	xor	a
	sub	a,-9(ix)
	ld	e,a
	ld	a,#0
	sbc	a,-8(ix)
	ld	d,a
	ld	a,#0
	sbc	a,-7(ix)
	ld	l,a
	ld	a,#0
	sbc	a,-6(ix)
	ld	h,a
	ld	sp,ix
	pop	ix
	ret
