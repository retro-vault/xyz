	.module	xcc_output
	.area	_DATA
_main__data_0:
	.ds	64
	.area	_CODE
_bench_mix16:
	; O3 sdcc-style leaf fast path: two-arg word mix helper
	ld	c, l
	ld	b, h
	ld	hl, #40503
	add	hl, de
	ld	a, c
	xor	a, l
	ld	c, a
	ld	a, b
	xor	a, h
	ld	b, a
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	a, e
	xor	#74
	ld	c, a
	ld	a, d
	xor	#127
	ld	b, a
	add	hl, bc
	ex	de, hl
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=23, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-23
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ld	a, l
	xor	#17
	ld	l, a
	ld	a, h
	xor	#0
	ld	h, a
	ld	-21(ix), l
	ld	-20(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	-23(ix), l
	ld	-22(ix), h
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	e, #66
	ld	d, #0
	add	hl, de
	ld	a, l
	; O3 bench-fill loop (count=64)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__data_0
__xcc_L6:
	ld	a, b
	cp	#64
	jr	nc, __xcc_L5
__xcc_L7:
	ld	a, c
	add	a, a
	add	a, a
	add	a, a
	xor	c
	ld	d, a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	xor	d
	add	a, #92
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L6
__xcc_L5:
	ld	hl, #48350
	ld	-18(ix), l
	ld	-17(ix), h
	xor	a
	ld	-19(ix), a
__xcc_L10:
	ld	a, -19(ix)
	cp	#56
	jr	z, __xcc_L11
	jr	c, __xcc_L11
	jp	__xcc_L13
__xcc_L11:
	ld	e, -19(ix)
	ld	d, #0
	ld	hl, #_main__data_0
	add	hl, de
	ld	a, (hl)
	ld	-20(ix), a
	ld	-11(ix), a
	ld	a, -20(ix)
	ld	-10(ix), a
	ld	a, #1
	ld	-12(ix), a
__xcc_L14:
	ld	a, -12(ix)
	cp	#8
	jr	nc, __xcc_L17
__xcc_L15:
	ld	a, -19(ix)
	ld	e, a
	ld	a, -12(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-20(ix), a
	ld	e, -20(ix)
	ld	d, #0
	ld	hl, #_main__data_0
	add	hl, de
	ld	a, (hl)
	ld	-9(ix), a
	ld	c, -11(ix)
	cp	c
	jr	nc, __xcc_L20
__xcc_L18:
	ld	a, -9(ix)
	ld	-11(ix), a
__xcc_L20:
	ld	a, -9(ix)
	ld	c, -10(ix)
	cp	c
	jr	z, __xcc_L16
	jr	c, __xcc_L16
__xcc_L21:
	ld	a, -9(ix)
	ld	-10(ix), a
__xcc_L23:
__xcc_L16:
	ld	a, -12(ix)
	add	a, #1
	ld	-12(ix), a
	jr	__xcc_L14
__xcc_L17:
	ld	a, -11(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -18(ix)
	ld	h, -17(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-7(ix), l
	ld	-6(ix), h
	ld	a, -10(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-5(ix), l
	ld	-4(ix), h
	ld	a, -11(ix)
	ld	-8(ix), a
	ld	a, -10(ix)
	ld	-3(ix), a
	ld	c, -8(ix)
	cp	c
	jr	z, __xcc_inl___xcc_L2_0
	jr	c, __xcc_inl___xcc_L2_0
__xcc_inl___xcc_L0_0:
	ld	a, -3(ix)
	ld	l, a
	ld	h, #0
	ld	-21(ix), l
	ld	-20(ix), h
	ld	a, -8(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	ld	d, b
	ld	e, c
	pop	hl
	or	a, a
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
	jr	__xcc_inl_exit_0
__xcc_inl___xcc_L2_0:
	ld	a, -8(ix)
	ld	l, a
	ld	h, #0
	ld	-21(ix), l
	ld	-20(ix), h
	ld	a, -3(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	ld	d, b
	ld	e, c
	pop	hl
	or	a, a
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_inl_exit_0:
	ld	e, -2(ix)
	ld	d, -1(ix)
	ld	l, -5(ix)
	ld	h, -4(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-18(ix), l
	ld	-17(ix), h
__xcc_L12:
	ld	a, -19(ix)
	add	a, #1
	ld	-19(ix), a
	jp	__xcc_L10
__xcc_L13:
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
