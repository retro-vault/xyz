	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	pop	de
	adc	hl, de
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	de, #2
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	pop	de
	adc	hl, de
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	pop	de
	adc	hl, de
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
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
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L5
__xcc_L5:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -36(ix)
	ld	h, -35(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -34(ix)
	ld	h, -33(ix)
	pop	de
	adc	hl, de
	ld	-36(ix), l
	ld	-35(ix), h
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
	push	ix
	pop	hl
	ld	de, #-40
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	hl
	ld	-6(ix), l
	ld	-5(ix), h
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
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L8
__xcc_L8:
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L11
__xcc_L9:
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L11
__xcc_L11:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -56(ix)
	ld	h, -55(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	pop	de
	adc	hl, de
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	push	ix
	pop	hl
	ld	de, #-60
	add	hl, de
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L14
__xcc_L12:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L14
__xcc_L14:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -68(ix)
	ld	h, -67(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	pop	de
	adc	hl, de
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	push	ix
	pop	hl
	ld	de, #-72
	add	hl, de
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L17
__xcc_L15:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L17
__xcc_L17:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
