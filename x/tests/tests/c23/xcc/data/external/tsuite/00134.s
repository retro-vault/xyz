	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=8)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	push	hl
	ld	hl, #32
	ld	b, l
	pop	hl
__shift_9383:
	ld	a, b
	or	a, a
	jp	z, __sdone_886
	add	hl, hl
	djnz	__shift_9383
__sdone_886:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #3
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	0(ix), l
	ld	1(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	2(ix), l
	ld	3(ix), h
	ld	hl, #1
	push	hl
	ld	hl, #32
	ld	b, l
	pop	hl
__shift_2777:
	ld	a, b
	or	a, a
	jp	z, __sdone_6915
	add	hl, hl
	djnz	__shift_2777
__sdone_6915:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -72(ix)
	ld	h, -71(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	push	hl
	ld	hl, #32
	ld	b, l
	pop	hl
__shift_6649:
	ld	a, b
	or	a, a
	jp	z, __sdone_1421
	add	hl, hl
	djnz	__shift_6649
__sdone_1421:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-84(ix), l
	ld	-83(ix), h
	ld	l, -90(ix)
	ld	h, -89(ix)
	push	hl
	ld	hl, #3
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -98(ix)
	ld	h, -97(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	push	hl
	ld	hl, #32
	ld	b, l
	pop	hl
__shift_2362:
	ld	a, b
	or	a, a
	jp	z, __sdone_27
	add	hl, hl
	djnz	__shift_2362
__sdone_27:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -106(ix)
	ld	h, -105(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -104(ix)
	ld	h, -103(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-112(ix), l
	ld	-111(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, -114(ix)
	ld	h, -113(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	l, -124(ix)
	ld	h, -123(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
