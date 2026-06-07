	.module	xcc_output
	.area	_DATA
_main__data_0:
	.ds	64
	.area	_CODE
_bench_mix16:
	; sdcccall(1) prologue: bench_mix16 (locals=0, temp_frame=10, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	-2(ix), l
	ld	-1(ix), h
	; keep incoming register arg t30 live in register for first use
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	ld	h, d
	ld	l, e
	; materialize incoming arg temp t30 for later reuse
	ld	-4(ix), e
	ld	-3(ix), d
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	h, b
	ld	l, c
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-6(ix), l
	ld	-5(ix), h
	ld	b, h
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-10(ix), l
	ld	-9(ix), h
	ex	de, hl
__bench_mix16_end:
	; epilogue: bench_mix16
	ld	sp, ix
	pop	ix
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=102, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-102
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
	ld	-2(ix), l
	ld	-1(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	h, b
	ld	l, c
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-4(ix), l
	ld	-3(ix), h
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
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	h, b
	ld	l, c
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-6(ix), l
	ld	-5(ix), h
	ld	de, #66
	add	hl, de
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-8(ix), a
	ld	-10(ix), a
	xor	a
	ld	-12(ix), a
	ld	hl, #_main__data_0
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L6:
	ld	a, -12(ix)
	cp	#64
	jp	nc, __xcc_L5
__xcc_L7:
	ld	l, -10(ix)
	ld	h, #0
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -10(ix)
	ld	h, #0
	push	hl
	ld	l, -16(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-18(ix), l
	ld	-17(ix), h
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
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -18(ix)
	ld	h, #0
	push	hl
	ld	l, -20(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-22(ix), l
	ld	-21(ix), h
	ld	a, -12(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #75
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-24(ix), l
	ld	-23(ix), h
	ld	de, #17
	add	hl, de
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-26(ix), a
	ld	l, -22(ix)
	ld	h, #0
	ld	e, -26(ix)
	ld	d, #0
	add	hl, de
	ld	-28(ix), l
	ld	-27(ix), h
	ld	a, -28(ix)
	ld	-10(ix), a
	ld	a, -12(ix)
	ld	l, a
	ld	h, #0
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -10(ix)
	ld	h, #0
	push	hl
	ld	l, -12(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	e, -30(ix)
	ld	d, -29(ix)
	add	hl, de
	ld	-34(ix), l
	ld	-33(ix), h
	push	hl
	ld	a, -32(ix)
	pop	hl
	ld	(hl), a
__xcc_L8:
	ld	l, -12(ix)
	ld	h, #0
	inc	hl
	ld	-36(ix), l
	ld	-35(ix), h
	ld	a, -36(ix)
	ld	-12(ix), a
	jp	__xcc_L6
__xcc_L5:
	ld	hl, #48350
	ld	-38(ix), l
	ld	-37(ix), h
	xor	a
	ld	-40(ix), a
	ld	hl, #_main__data_0
	ld	-42(ix), l
	ld	-41(ix), h
	ld	hl, #_main__data_0
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #_main__data_0
	ld	-46(ix), l
	ld	-45(ix), h
	ld	hl, #_main__data_0
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #_main__data_0
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, #_main__data_0
	ld	-52(ix), l
	ld	-51(ix), h
__xcc_L10:
	ld	a, -40(ix)
	cp	#56
	jr	z, __xcc_L11
	jr	c, __xcc_L11
	jp	__xcc_L13
__xcc_L11:
	ld	a, -40(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-54(ix), l
	ld	-53(ix), h
	ld	a, (hl)
	ld	-56(ix), a
	ld	-58(ix), a
	ld	a, -40(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-60(ix), l
	ld	-59(ix), h
	ld	a, (hl)
	ld	-62(ix), a
	ld	-64(ix), a
	ld	a, #1
	ld	-66(ix), a
__xcc_L14:
	ld	a, -66(ix)
	cp	#8
	jp	nc, __xcc_L17
__xcc_L15:
	ld	l, -40(ix)
	ld	h, #0
	ld	e, -66(ix)
	ld	d, #0
	add	hl, de
	ld	-68(ix), l
	ld	-67(ix), h
	ld	a, -68(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-70(ix), l
	ld	-69(ix), h
	ld	a, (hl)
	ld	-72(ix), a
	ld	c, -58(ix)
	cp	c
	jr	nc, __xcc_L20
__xcc_L18:
	ld	a, -68(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-74(ix), l
	ld	-73(ix), h
	ld	a, (hl)
	ld	-76(ix), a
	ld	-58(ix), a
__xcc_L20:
	ld	a, -68(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -50(ix)
	ld	h, -49(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-78(ix), l
	ld	-77(ix), h
	ld	a, (hl)
	ld	-80(ix), a
	ld	c, -64(ix)
	cp	c
	jr	z, __xcc_L16
	jr	c, __xcc_L16
__xcc_L21:
	ld	a, -68(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -52(ix)
	ld	h, -51(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-82(ix), l
	ld	-81(ix), h
	ld	a, (hl)
	ld	-84(ix), a
	ld	-64(ix), a
__xcc_L23:
__xcc_L16:
	ld	l, -66(ix)
	ld	h, #0
	inc	hl
	ld	-86(ix), l
	ld	-85(ix), h
	ld	a, -86(ix)
	ld	-66(ix), a
	jp	__xcc_L14
__xcc_L17:
	ld	a, -58(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -38(ix)
	ld	h, -37(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-88(ix), l
	ld	-87(ix), h
	ld	a, -64(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -88(ix)
	ld	h, -87(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-90(ix), l
	ld	-89(ix), h
	ld	a, -64(ix)
	ld	c, -58(ix)
	cp	c
	jr	z, __xcc_inl___xcc_L2_0
	jr	c, __xcc_inl___xcc_L2_0
__xcc_inl___xcc_L0_0:
	ld	a, -64(ix)
	ld	l, a
	ld	h, #0
	ld	-92(ix), l
	ld	-91(ix), h
	ld	a, -58(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -92(ix)
	ld	h, -91(ix)
	ld	d, b
	ld	e, c
	or	a, a
	sbc	hl, de
	ld	-94(ix), l
	ld	-93(ix), h
	ld	-96(ix), l
	ld	-95(ix), h
	jr	__xcc_inl_exit_0
__xcc_inl___xcc_L2_0:
	ld	a, -58(ix)
	ld	l, a
	ld	h, #0
	ld	-98(ix), l
	ld	-97(ix), h
	ld	a, -64(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -98(ix)
	ld	h, -97(ix)
	ld	d, b
	ld	e, c
	or	a, a
	sbc	hl, de
	ld	-100(ix), l
	ld	-99(ix), h
	ld	-96(ix), l
	ld	-95(ix), h
__xcc_inl_exit_0:
	ld	e, -96(ix)
	ld	d, -95(ix)
	ld	l, -90(ix)
	ld	h, -89(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-38(ix), l
	ld	-37(ix), h
__xcc_L12:
	ld	l, -40(ix)
	ld	h, #0
	inc	hl
	ld	-102(ix), l
	ld	-101(ix), h
	ld	a, -102(ix)
	ld	-40(ix), a
	jp	__xcc_L10
__xcc_L13:
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
