	; fixed16_16_mul.s
	; Signed 16.16 fixed-point multiply:
	; result = (a * b) >> 16
	; Uses 16-bit partial products:
	; a = ah * 65536 + al
	; b = bh * 65536 + bl
	; (a*b)>>16 = ah*bl + bh*al + high16(al*bl)
	; + (low16(ah*bh) << 16)
	; Public ABI is int32 raw fixed (DE low16, HL high16).
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_mul
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_mul
	.area	_CODE
	; locals:
	; -12..-9    acc32, low..high
	; -8..-7     a low word
	; -6..-5     a high word
	; -4..-3     b low word
	; -2..-1     b high word
	; inputs:  DE:HL = a, 4(ix)..7(ix) = b
	; outputs: DE:HL = signed ((a * b) >> 16)
_fixed16_16_mul::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	b,h
	ld	c,l
	ld	hl,#-12
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	ld	-8(ix),e
	ld	-7(ix),d
	ld	-6(ix),l
	ld	-5(ix),h
	ld	a,4(ix)
	ld	-4(ix),a
	ld	a,5(ix)
	ld	-3(ix),a
	ld	a,6(ix)
	ld	-2(ix),a
	ld	a,7(ix)
	ld	-1(ix),a
	; acc = high16(al * bl)
	ld	l,-8(ix)
	ld	h,-7(ix)
	ld	e,-4(ix)
	ld	d,-3(ix)
	call	.mul_u16
	ld	-12(ix),l
	ld	-11(ix),h
	xor	a
	ld	-10(ix),a
	ld	-9(ix),a
	; acc += ah * bl
	ld	l,-6(ix)
	ld	h,-5(ix)
	ld	e,-4(ix)
	ld	d,-3(ix)
	call	.mul_su16
	call	.add_dehl_to_acc
	; acc += bh * al
	ld	l,-2(ix)
	ld	h,-1(ix)
	ld	e,-8(ix)
	ld	d,-7(ix)
	call	.mul_su16
	call	.add_dehl_to_acc
	; acc += low16(ah * bh) << 16
	ld	l,-6(ix)
	ld	h,-5(ix)
	ld	e,-2(ix)
	ld	d,-1(ix)
	call	.mul_u16
	ld	a,-10(ix)
	add	a,e
	ld	-10(ix),a
	ld	a,-9(ix)
	adc	a,d
	ld	-9(ix),a
	ld	e,-12(ix)
	ld	d,-11(ix)
	ld	l,-10(ix)
	ld	h,-9(ix)
	ld	sp,ix
	pop	ix
	ret
.add_dehl_to_acc:
	ld	a,-12(ix)
	add	a,e
	ld	-12(ix),a
	ld	a,-11(ix)
	adc	a,d
	ld	-11(ix),a
	ld	a,-10(ix)
	adc	a,l
	ld	-10(ix),a
	ld	a,-9(ix)
	adc	a,h
	ld	-9(ix),a
	ret
	; Signed-by-unsigned 16x16 -> signed 32.
	; inputs: HL = signed, DE = unsigned
	; output: DE:HL = low:high product
.mul_su16:
	bit	7,h
	jr	z,.mul_u16
	xor	a
	sub	a,l
	ld	l,a
	ld	a,#0
	sbc	a,h
	ld	h,a
	call	.mul_u16
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
	; Unsigned 16x16 -> 32.
	; inputs: HL = multiplier, DE = multiplicand
	; output: DE:HL = low:high product
.mul_u16:
	ld	iy,#0
	ld	b,#16
.mul_u16_loop:
	add	iy,iy
	adc	hl,hl
	jr	nc,.mul_u16_skip
	add	iy,de
	jr	nc,.mul_u16_skip
	inc	hl
.mul_u16_skip:
	djnz	.mul_u16_loop
	push	iy
	pop	de
	ret
