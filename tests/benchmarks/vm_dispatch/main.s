	.module	xcc_output
	.area	_DATA
_main__code_0:
	.ds	96
_main__mem_1:
	.ds	16
	.area	_CODE
_bench_seed_byte:
	; sdcccall(1) prologue: bench_seed_byte (locals=0, temp_frame=16, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	-2(ix), a
	ld	hl, #-16
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	ld	hl, (#65296)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	a, -2(ix)
	ld	l, a
	ld	h, #0
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ex	de, hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-8(ix), l
	ld	-7(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -8(ix)
	ld	h, -7(ix)
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
	ld	-10(ix), l
	ld	-9(ix), h
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
	ld	l, -10(ix)
	ld	h, -9(ix)
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #49
	ld	e, -6(ix)
	ld	d, -5(ix)
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-14(ix), l
	ld	-13(ix), h
	ld	a, l
	ld	-16(ix), a
__bench_seed_byte_end:
	; epilogue: bench_seed_byte
	ld	sp, ix
	pop	ix
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=188, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-188
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	push	hl
	ld	a, #6
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-2(ix), a
	ld	-4(ix), a
	xor	a
	ld	-6(ix), a
	ld	hl, #_main__code_0
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L6:
	ld	a, -6(ix)
	cp	#96
	jp	nc, __xcc_L10
__xcc_L7:
	ld	l, -4(ix)
	ld	h, #0
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -4(ix)
	ld	h, #0
	push	hl
	ld	l, -10(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-12(ix), l
	ld	-11(ix), h
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
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -12(ix)
	ld	h, #0
	push	hl
	ld	l, -14(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-16(ix), l
	ld	-15(ix), h
	ld	a, -6(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #92
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-18(ix), l
	ld	-17(ix), h
	ld	de, #17
	add	hl, de
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-20(ix), a
	ld	l, -16(ix)
	ld	h, #0
	ld	e, -20(ix)
	ld	d, #0
	add	hl, de
	ld	-22(ix), l
	ld	-21(ix), h
	ld	a, -22(ix)
	ld	-4(ix), a
	ld	a, -6(ix)
	ld	l, a
	ld	h, #0
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -4(ix)
	ld	h, #0
	push	hl
	ld	l, -6(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	e, -24(ix)
	ld	d, -23(ix)
	add	hl, de
	ld	-28(ix), l
	ld	-27(ix), h
	push	hl
	ld	a, -26(ix)
	pop	hl
	ld	(hl), a
__xcc_L8:
	ld	l, -6(ix)
	ld	h, #0
	inc	hl
	ld	-30(ix), l
	ld	-29(ix), h
	ld	a, -30(ix)
	ld	-6(ix), a
	jp	__xcc_L6
__xcc_L10:
	push	hl
	ld	a, #55
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-32(ix), a
	ld	-34(ix), a
	xor	a
	ld	-36(ix), a
	ld	hl, #_main__mem_1
	ld	-38(ix), l
	ld	-37(ix), h
__xcc_L13:
	ld	a, -36(ix)
	cp	#16
	jp	nc, __xcc_L12
__xcc_L14:
	ld	l, -34(ix)
	ld	h, #0
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -34(ix)
	ld	h, #0
	push	hl
	ld	l, -40(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-42(ix), l
	ld	-41(ix), h
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
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -42(ix)
	ld	h, #0
	push	hl
	ld	l, -44(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-46(ix), l
	ld	-45(ix), h
	ld	a, -36(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #109
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-48(ix), l
	ld	-47(ix), h
	ld	de, #17
	add	hl, de
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-50(ix), a
	ld	l, -46(ix)
	ld	h, #0
	ld	e, -50(ix)
	ld	d, #0
	add	hl, de
	ld	-52(ix), l
	ld	-51(ix), h
	ld	a, -52(ix)
	ld	-34(ix), a
	ld	a, -36(ix)
	ld	l, a
	ld	h, #0
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -34(ix)
	ld	h, #0
	push	hl
	ld	l, -36(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	e, -54(ix)
	ld	d, -53(ix)
	add	hl, de
	ld	-58(ix), l
	ld	-57(ix), h
	push	hl
	ld	a, -56(ix)
	pop	hl
	ld	(hl), a
__xcc_L15:
	ld	l, -36(ix)
	ld	h, #0
	inc	hl
	ld	-60(ix), l
	ld	-59(ix), h
	ld	a, -60(ix)
	ld	-36(ix), a
	jp	__xcc_L13
__xcc_L12:
	xor	a
	ld	-62(ix), a
	push	hl
	ld	a, #1
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-64(ix), a
	ld	-66(ix), a
	push	hl
	ld	a, #2
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-68(ix), a
	ld	-70(ix), a
	push	hl
	ld	a, #3
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-72(ix), a
	ld	-74(ix), a
	ld	hl, #52719
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, #_main__code_0
	ld	-78(ix), l
	ld	-77(ix), h
	ld	hl, #_main__mem_1
	ld	-80(ix), l
	ld	-79(ix), h
	ld	hl, #_main__code_0
	ld	-82(ix), l
	ld	-81(ix), h
	ld	hl, #_main__mem_1
	ld	-84(ix), l
	ld	-83(ix), h
	ld	hl, #_main__code_0
	ld	-86(ix), l
	ld	-85(ix), h
	ld	hl, #_main__mem_1
	ld	-88(ix), l
	ld	-87(ix), h
	ld	hl, #_main__mem_1
	ld	-90(ix), l
	ld	-89(ix), h
__xcc_L17:
	ld	a, -62(ix)
	cp	#96
	jp	nc, __xcc_L19
__xcc_L18:
	ld	a, -62(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-92(ix), l
	ld	-91(ix), h
	ld	a, (hl)
	ld	-94(ix), a
	and	#7
	ld	-96(ix), a
	cp	#0
	jr	z, __xcc_L20
	ld	a, -96(ix)
	cp	#1
	jp	z, __xcc_L21
	ld	a, -96(ix)
	cp	#2
	jp	z, __xcc_L22
	ld	a, -96(ix)
	cp	#3
	jp	z, __xcc_L23
	ld	a, -96(ix)
	cp	#4
	jp	z, __xcc_L24
	ld	a, -96(ix)
	cp	#5
	jp	z, __xcc_L25
	ld	a, -96(ix)
	cp	#6
	jp	z, __xcc_L26
	jp	__xcc_L27
__xcc_L20:
	ld	a, -62(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -82(ix)
	ld	h, -81(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-98(ix), l
	ld	-97(ix), h
	ld	a, (hl)
	ld	-100(ix), a
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, l
	and	#15
	ld	l, a
	ld	a, h
	and	#0
	ld	h, a
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	ld	e, -102(ix)
	ld	d, -101(ix)
	add	hl, de
	ld	b, h
	ld	c, l
	ld	a, (hl)
	ld	-104(ix), a
	ld	l, -66(ix)
	ld	h, #0
	ld	e, -104(ix)
	ld	d, #0
	add	hl, de
	ld	-106(ix), l
	ld	-105(ix), h
	ld	a, -106(ix)
	ld	-66(ix), a
	jp	__xcc_L28
__xcc_L21:
	ld	a, -62(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -86(ix)
	ld	h, -85(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-108(ix), l
	ld	-107(ix), h
	ld	a, (hl)
	ld	-110(ix), a
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	sra	h
	rr	l
	ld	-112(ix), l
	ld	-111(ix), h
	ld	a, l
	and	#15
	ld	l, a
	ld	a, h
	and	#0
	ld	h, a
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-114(ix), a
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -84(ix)
	ld	h, -83(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-116(ix), l
	ld	-115(ix), h
	ld	a, (hl)
	ld	-118(ix), a
	ld	l, -66(ix)
	ld	h, #0
	push	hl
	ld	l, -118(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-120(ix), l
	ld	-119(ix), h
	ld	a, -120(ix)
	ld	-66(ix), a
	jp	__xcc_L28
__xcc_L22:
	ld	a, -70(ix)
	ld	l, a
	ld	h, #0
	ld	-122(ix), l
	ld	-121(ix), h
	ld	a, -66(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -122(ix)
	ld	h, -121(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-124(ix), l
	ld	-123(ix), h
	inc	hl
	ld	b, h
	ld	c, l
	ld	a, l
	ld	-126(ix), a
	ld	-70(ix), a
	jp	__xcc_L28
__xcc_L23:
	ld	l, -66(ix)
	ld	h, #0
	ld	e, -70(ix)
	ld	d, #0
	add	hl, de
	ld	-128(ix), l
	ld	-127(ix), h
	ld	l, -74(ix)
	ld	h, #0
	push	hl
	ld	l, -128(ix)
	ld	h, #0
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-130
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-130
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	-74(ix), a
	jp	__xcc_L28
__xcc_L24:
	ld	a, -62(ix)
	and	#15
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-132
	add	hl, bc
	ld	(hl), a
	pop	bc
	pop	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-132
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -88(ix)
	ld	h, -87(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-134
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-134
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, (hl)
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-136
	add	hl, bc
	ld	(hl), a
	pop	bc
	pop	af
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-136
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	l, a
	pop	af
	ld	h, #0
	ld	e, -74(ix)
	ld	d, #0
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-138
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	a, -62(ix)
	and	#15
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-140
	add	hl, bc
	ld	(hl), a
	pop	bc
	pop	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-140
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -90(ix)
	ld	h, -89(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-142
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-142
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	push	hl
	push	bc
	push	ix
	pop	hl
	ld	bc, #-138
	add	hl, bc
	ld	a, (hl)
	pop	bc
	pop	hl
	ld	(hl), a
	jp	__xcc_L28
__xcc_L25:
	ld	a, -66(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, l
	and	#1
	ld	l, a
	ld	a, h
	and	#0
	ld	h, a
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-144
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-144
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ex	de, hl
	ld	hl, #0
	or	a, a
	sbc	hl, de
	ld	hl, #1
	jr	nz, __cmp_e_89383
	dec	hl
__cmp_e_89383:
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	z, __xcc_L32
__xcc_L33:
	ld	a, -62(ix)
	cp	#94
	ld	hl, #1
	jr	c, __cmp_e_30886
	dec	hl
__cmp_e_30886:
	ld	b, h
	ld	c, l
	ex	de, hl
	ld	hl, #0
	or	a, a
	sbc	hl, de
	ld	hl, #1
	jr	nz, __cmp_e_92777
	dec	hl
__cmp_e_92777:
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-146
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	jr	__xcc_L34
__xcc_L32:
	ld	hl, #0
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-146
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
__xcc_L34:
	push	bc
	push	ix
	pop	hl
	ld	bc, #-146
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, h
	or	a, l
	jp	z, __xcc_L28
__xcc_L29:
	ld	l, -62(ix)
	ld	h, #0
	inc	hl
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-148
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-148
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	-62(ix), a
__xcc_L31:
	jp	__xcc_L28
__xcc_L26:
	ld	a, -66(ix)
	ld	l, a
	ld	h, #0
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-150
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-150
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	add	hl, hl
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-152
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-150
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	b, #7
__shift_6915:
	sra	h
	rr	l
	djnz	__shift_6915
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-152
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	push	hl
	ld	h, b
	ld	l, c
	pop	de
	ld	a, l
	or	a, e
	ld	l, a
	ld	a, h
	or	a, d
	ld	h, a
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-154
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-154
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, l
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-156
	add	hl, bc
	ld	(hl), a
	pop	bc
	pop	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-156
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	-66(ix), a
	jp	__xcc_L28
__xcc_L27:
	ld	a, -66(ix)
	ld	l, a
	ld	h, #0
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-158
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	a, -70(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-158
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-160
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	a, -74(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-160
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-162
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-162
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, l
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-164
	add	hl, bc
	ld	(hl), a
	pop	bc
	pop	af
	push	bc
	push	ix
	pop	hl
	ld	bc, #-164
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	-66(ix), a
__xcc_L28:
	ld	a, -70(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	h, l
	ld	l, #0
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-166
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	a, -66(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	push	hl
	push	bc
	push	ix
	pop	hl
	ld	bc, #-166
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	pop	de
	ld	a, l
	or	a, e
	ld	l, a
	ld	a, h
	or	a, d
	ld	h, a
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-168
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-170
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-168
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-170
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
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
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-172
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-172
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
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
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-174
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-168
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-174
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-176
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	ld	a, -74(ix)
	ld	l, a
	ld	h, #0
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-178
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-178
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-176
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
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
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-180
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-180
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
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
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-182
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-178
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	push	bc
	push	ix
	pop	hl
	ld	bc, #-182
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	d, b
	ld	e, c
	add	hl, de
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-184
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-184
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	pop	bc
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -62(ix)
	ld	h, #0
	inc	hl
	push	de
	ld	d, h
	ld	e, l
	push	ix
	pop	hl
	ld	bc, #-186
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	pop	de
	push	bc
	push	ix
	pop	hl
	ld	bc, #-186
	add	hl, bc
	ld	a, (hl)
	pop	bc
	ld	-62(ix), a
	jp	__xcc_L17
__xcc_L19:
	ld	l, -76(ix)
	ld	h, -75(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
