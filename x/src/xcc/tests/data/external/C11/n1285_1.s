	.module xcc_output


	.area _CODE

	.globl _func_return
_func_return:
	; prologue: func_return (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	.globl _f
	call	_f
	ld	-12(ix), l
	ld	-11(ix), h
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	jp	__func_return_end
__func_return_end:
	; epilogue: func_return
	ld	sp, ix
	pop	ix
	ret
	.globl _ternary
_ternary:
	; prologue: ternary (locals=22)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-22
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -10(ix)
	ld	h, -9(ix)
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	jp	__xcc_L2
__xcc_L1:
	.globl _f
	call	_f
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	-26(ix), l
	ld	-25(ix), h
__xcc_L2:
	push	ix
	pop	hl
	ld	de, #-26
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	(_p), hl
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	push	ix
	pop	hl
	ld	de, #-22
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-22
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	jp	__xcc_L5
__xcc_L4:
	.globl _f
	call	_f
	ld	-58(ix), l
	ld	-57(ix), h
	push	ix
	pop	hl
	ld	de, #-58
	add	hl, de
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	ld	-48(ix), l
	ld	-47(ix), h
__xcc_L5:
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, (_p)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	ld	e, -66(ix)
	ld	d, -65(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	jp	__ternary_end
__ternary_end:
	; epilogue: ternary
	ld	sp, ix
	pop	ix
	ret
	.globl _comma
_comma:
	; prologue: comma (locals=10)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	(_p), hl
	ld	hl, (_p)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	jp	__comma_end
__comma_end:
	; epilogue: comma
	ld	sp, ix
	pop	ix
	ret
	.globl _cast
_cast:
	; prologue: cast (locals=10)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-20(ix), l
	ld	-19(ix), h
	push	ix
	pop	hl
	ld	de, #-20
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	(_p), hl
	ld	hl, (_p)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	jp	__cast_end
__cast_end:
	; epilogue: cast
	ld	sp, ix
	pop	ix
	ret
	.globl _assign
_assign:
	; prologue: assign (locals=20)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-20
	add	hl, sp
	ld	sp, hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	(_p), hl
	ld	hl, (_p)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	jp	__assign_end
__assign_end:
	; epilogue: assign
	ld	sp, ix
	pop	ix
	ret
