	.module	xcc_output
	.area	_DATA
_f__input_0:
	.ds	128
_f__bins_1:
	.ds	16
	.area	_CODE
	.globl	_f
_f:
	; sdcccall(1) prologue: f (locals=0, temp_frame=7, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-7
	add	hl, sp
	ld	sp, hl
	; O3 byte zero loop (count=16)
	ld	hl, #_f__bins_1
	ld	b, #16
	xor	a
__xcc_L0:
__xcc_L1:
	ld	(hl), a
	inc	hl
	djnz	__xcc_L1
__xcc_L3:
	xor	a
	ld	-5(ix), a
__xcc_L4:
	ld	a, -5(ix)
	cp	#128
	jr	nc, __xcc_L7
__xcc_L5:
	ld	e, -5(ix)
	ld	d, #0
	ld	hl, #_f__input_0
	add	hl, de
	ld	a, (hl)
	ld	-6(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-7(ix), a
	ld	e, -7(ix)
	ld	d, #0
	ld	hl, #_f__bins_1
	add	hl, de
	ld	a, (hl)
	add	a, #1
	ld	-7(ix), a
	ld	a, -6(ix)
	srl	a
	srl	a
	srl	a
	srl	a
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #_f__bins_1
	add	hl, bc
	ld	a, -7(ix)
	ld	(hl), a
__xcc_L6:
	ld	a, -5(ix)
	add	a, #1
	ld	-5(ix), a
	jr	__xcc_L4
__xcc_L7:
	ld	hl, #26505
	ld	-2(ix), l
	ld	-1(ix), h
	xor	a
	ld	-5(ix), a
__xcc_L8:
	ld	a, -5(ix)
	cp	#16
	jr	nc, __xcc_L11
__xcc_L9:
	ld	e, -5(ix)
	ld	d, #0
	ld	hl, #_f__bins_1
	add	hl, de
	ld	a, (hl)
	ld	-6(ix), a
	ld	a, -5(ix)
	ld	h, a
	ld	a, -6(ix)
	ld	l, a
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	add	hl, bc
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L10:
	ld	a, -5(ix)
	add	a, #1
	ld	-5(ix), a
	jr	__xcc_L8
__xcc_L11:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__f_end:
	; epilogue: f
	ld	sp, ix
	pop	ix
	ret
