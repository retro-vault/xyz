	; fixed8_8_math.s
	; Fixed 8.8 implementations for decomposition, scaling, remainder,
	; and composed arithmetic float math entry points.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed8_8_math
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed8_8_ldexp
	.globl	_fixed8_8_scalbn
	.globl	_fixed8_8_scalbln
	.globl	_fixed8_8_ilogb
	.globl	_fixed8_8_logb
	.globl	_fixed8_8_significand
	.globl	_fixed8_8_frexp
	.globl	_fixed8_8_modf
	.globl	_fixed8_8_fma
	.globl	_fixed8_8_hypot
	.globl	_fixed8_8_fmod
	.globl	_fixed8_8_remainder
	.globl	_fixed8_8_remquo
	.globl	_fixed8_8_abs
	.globl	_fixed8_8_add
	.globl	_fixed8_8_div
	.globl	_fixed8_8_from_int
	.globl	_fixed8_8_mul
	.globl	_fixed8_8_round
	.globl	_fixed8_8_sqrt
	.globl	_fixed8_8_sub
	.globl	_fixed8_8_to_int
	.globl	_fixed8_8_trunc
XLO	.equ	-6
XHI	.equ	-5
YLO	.equ	-4
YHI	.equ	-3
TLO	.equ	-2
THI	.equ	-1
	.area	_CODE
_fixed8_8_scalbn::
_fixed8_8_ldexp::
	bit	7,d
	jr	nz,.shift_right
	ld	b,e
	ld	a,b
	or	a
	jr	z,.ldexp_done
.left_loop:
	add	hl,hl
	djnz	.left_loop
	jr	.ldexp_done
.shift_right:
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	ld	b,e
	ld	a,b
	or	a
	jr	z,.ldexp_done
.right_loop:
	sra	h
	rr	l
	djnz	.right_loop
.ldexp_done:
	ex	de,hl
	ret
_fixed8_8_scalbln::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	e,4(ix)
	ld	d,5(ix)
	pop	ix
	jr	_fixed8_8_ldexp
_fixed8_8_ilogb::
	call	_fixed8_8_abs
	ld	h,d
	ld	l,e
	ld	a,h
	or	l
	jr	nz,.ilogb_nonzero
	ld	de,#0x8000
	ret
.ilogb_nonzero:
	ld	b,#0
.ilogb_loop:
	inc	b
	srl	h
	rr	l
	ld	a,h
	or	l
	jr	nz,.ilogb_loop
	ld	a,b
	sub	#9	; bit_index - 8 fractional bits
	ld	e,a
	rlca
	sbc	a,a
	ld	d,a
	ret
_fixed8_8_logb::
	call	_fixed8_8_ilogb
	ld	a,d
	cp	#0x80
	jr	nz,.logb_convert
	ld	de,#0x8000
	ret
.logb_convert:
	ld	h, d
	ld	l, e
	jp	_fixed8_8_from_int
_fixed8_8_significand::
	ld	a,h
	or	l
	jr	nz,.significand_nonzero
	ld	de,#0
	ret
.significand_nonzero:
	push	hl
	call	_fixed8_8_ilogb
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	pop	hl
	jp	_fixed8_8_ldexp
_fixed8_8_frexp::
	ld	a,h
	or	l
	jr	nz,.frexp_nonzero
	ld	a,d
	or	e
	jr	z,.frexp_zero_ret
	xor	a
	ld	(de),a
	inc	de
	ld	(de),a
.frexp_zero_ret:
	ld	de,#0
	ret
.frexp_nonzero:
	push	hl
	push	de
	call	_fixed8_8_ilogb
	inc	de
	pop	bc
	ld	a,c
	or	b
	jr	z,.frexp_store_done
	ld	a,e
	ld	(bc),a
	inc	bc
	ld	a,d
	ld	(bc),a
.frexp_store_done:
	xor	a
	sub	a,e
	ld	e,a
	ld	a,#0
	sbc	a,d
	ld	d,a
	pop	hl
	jp	_fixed8_8_ldexp
_fixed8_8_modf::
	push	hl
	push	de
	call	_fixed8_8_trunc
	pop	bc
	ld	a,c
	or	b
	jr	z,.modf_store_done
	ld	a,e
	ld	(bc),a
	inc	bc
	ld	a,d
	ld	(bc),a
.modf_store_done:
	pop	hl
	jp	_fixed8_8_sub
_fixed8_8_fma::
	push	ix
	ld	ix,#0
	add	ix,sp
	call	_fixed8_8_mul
	push	de
	ld	e,4(ix)
	ld	d,5(ix)
	pop	hl
	call	_fixed8_8_add
	pop	ix
	ret
_fixed8_8_hypot::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	b,h
	ld	c,l
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	ld	XLO(ix),l
	ld	XHI(ix),h
	ld	YLO(ix),e
	ld	YHI(ix),d
	ld	e,XLO(ix)
	ld	d,XHI(ix)
	ld	l,e
	ld	h,d
	call	_fixed8_8_mul
	ld	TLO(ix),e
	ld	THI(ix),d
	ld	e,YLO(ix)
	ld	d,YHI(ix)
	ld	l,e
	ld	h,d
	call	_fixed8_8_mul
	ld	l,TLO(ix)
	ld	h,THI(ix)
	call	_fixed8_8_add
	ld	h, d
	ld	l, e
	call	_fixed8_8_sqrt
	ld	sp,ix
	pop	ix
	ret
_fixed8_8_fmod::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	b,h
	ld	c,l
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	ld	XLO(ix),l
	ld	XHI(ix),h
	ld	YLO(ix),e
	ld	YHI(ix),d
	call	.quotient_trunc_frame
	jp	.finish_remainder
_fixed8_8_remainder::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	b,h
	ld	c,l
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	ld	XLO(ix),l
	ld	XHI(ix),h
	ld	YLO(ix),e
	ld	YHI(ix),d
	call	.quotient_round_frame
	jp	.finish_remainder
_fixed8_8_remquo::
	push	ix
	ld	ix,#0
	add	ix,sp
	ld	b,h
	ld	c,l
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	h,b
	ld	l,c
	ld	XLO(ix),l
	ld	XHI(ix),h
	ld	YLO(ix),e
	ld	YHI(ix),d
	ld	a,e
	or	d
	jr	nz,.remquo_div
	ld	de,#0
	jr	.remquo_done
.remquo_div:
	call	.quotient_round_frame
	ld	TLO(ix),e
	ld	THI(ix),d
	ld	h, d
	ld	l, e
	call	_fixed8_8_to_int
	ld	c,4(ix)
	ld	b,5(ix)
	ld	a,b
	or	c
	jr	z,.remquo_store_done
	ld	a,e
	ld	(bc),a
	inc	bc
	ld	a,d
	ld	(bc),a
.remquo_store_done:
	ld	e,TLO(ix)
	ld	d,THI(ix)
	call	.finish_remainder_frame
.remquo_done:
	ld	sp,ix
	pop	ix
	ret
.quotient_trunc_frame:
	ld	a,YLO(ix)
	or	YHI(ix)
	jr	nz,.qt_nonzero
	ld	de,#0
	ret
.qt_nonzero:
	ld	e,YLO(ix)
	ld	d,YHI(ix)
	ld	l,XLO(ix)
	ld	h,XHI(ix)
	call	_fixed8_8_div
	ld	h, d
	ld	l, e
	jp	_fixed8_8_trunc
.quotient_round_frame:
	ld	a,YLO(ix)
	or	YHI(ix)
	jr	nz,.qr_nonzero
	ld	de,#0
	ret
.qr_nonzero:
	ld	e,YLO(ix)
	ld	d,YHI(ix)
	ld	l,XLO(ix)
	ld	h,XHI(ix)
	call	_fixed8_8_div
	ld	h, d
	ld	l, e
	jp	_fixed8_8_round
.finish_remainder:
	call	.finish_remainder_frame
	ld	sp,ix
	pop	ix
	ret
.finish_remainder_frame:
	ld	TLO(ix),e
	ld	THI(ix),d
	ld	e,YLO(ix)
	ld	d,YHI(ix)
	ld	l,TLO(ix)
	ld	h,THI(ix)
	call	_fixed8_8_mul
	ld	l,XLO(ix)
	ld	h,XHI(ix)
	jp	_fixed8_8_sub
