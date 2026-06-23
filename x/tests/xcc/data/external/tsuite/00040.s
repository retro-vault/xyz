	.module xcc_output

	.area _DATA
	.globl _N
_N:
	.ds 2
	.globl _t
_t:
	.ds 2


	.area _CODE

	.globl _chk
_chk:
	; prologue: chk (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive param x at 4(ix)
	; receive param y at 6(ix)
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L0:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #8
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	.globl __mul16
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, (_t)
	ld	e, -12(ix)
	ld	d, -11(ix)
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
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -16(ix)
	ld	d, -15(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __mul16
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -20(ix)
	ld	d, -19(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, (_t)
	ld	e, -24(ix)
	ld	d, -23(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -28(ix)
	ld	d, -27(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #8
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	ld	hl, #8
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
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	l, -38(ix)
	ld	h, -37(ix)
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L6
__xcc_L4:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	.globl __mul16
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	e, -46(ix)
	ld	d, -45(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, (_t)
	ld	e, -50(ix)
	ld	d, -49(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -54(ix)
	ld	d, -53(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L6
__xcc_L6:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	hl, #8
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
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	l, -64(ix)
	ld	h, -63(ix)
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L7
	jp	__xcc_L9
__xcc_L7:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	.globl __mul16
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	e, -72(ix)
	ld	d, -71(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, (_t)
	ld	e, -76(ix)
	ld	d, -75(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -80(ix)
	ld	d, -79(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L9
__xcc_L9:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	ld	hl, #8
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
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	push	hl
	ld	l, -90(ix)
	ld	h, -89(ix)
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	jp	__xcc_L12
__xcc_L10:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	.globl __mul16
	ld	l, -96(ix)
	ld	h, -95(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	ld	e, -98(ix)
	ld	d, -97(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	ld	hl, (_t)
	ld	e, -102(ix)
	ld	d, -101(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	l, -104(ix)
	ld	h, -103(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -106(ix)
	ld	d, -105(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, -108(ix)
	ld	h, -107(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L12
__xcc_L12:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -110(ix)
	ld	h, -109(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	l, -112(ix)
	ld	h, -111(ix)
	push	hl
	ld	l, -116(ix)
	ld	h, -115(ix)
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	jp	__xcc_L15
__xcc_L13:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	.globl __mul16
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	ld	e, -124(ix)
	ld	d, -123(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	hl, (_t)
	ld	e, -128(ix)
	ld	d, -127(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-130(ix), l
	ld	-129(ix), h
	ld	l, -130(ix)
	ld	h, -129(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -132(ix)
	ld	d, -131(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-134(ix), l
	ld	-133(ix), h
	ld	l, -134(ix)
	ld	h, -133(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L15
__xcc_L15:
__xcc_L2:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-136(ix), l
	ld	-135(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__chk_end
__chk_end:
	; epilogue: chk
	ld	sp, ix
	pop	ix
	ret
	.globl _go
_go:
	; prologue: go (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param n at 4(ix)
	; receive param x at 6(ix)
	; receive param y at 8(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89172
	ld	hl, #0
	jp	__cmp_e_55736
__cmp_t_89172:
	ld	hl, #1
__cmp_e_55736:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L16
	jp	__xcc_L18
__xcc_L16:
	ld	hl, (_N)
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, (_N)
	inc	hl
	ld	(_N), hl
	ld	hl, #0
	jp	__go_end
	jp	__xcc_L18
__xcc_L18:
__xcc_L19:
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	hl, #8
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L20
	jp	__xcc_L22
__xcc_L20:
__xcc_L23:
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	hl, #8
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_2567
	ld	hl, #0
	jp	__cmp_e_56429
__cmp_t_2567:
	ld	hl, #1
__cmp_e_56429:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L24
	jp	__xcc_L26
__xcc_L24:
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	.globl _chk
	call	_chk
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_65782
	ld	hl, #0
	jp	__cmp_e_21530
__cmp_t_65782:
	ld	hl, #1
__cmp_e_21530:
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L27
	jp	__xcc_L29
__xcc_L27:
	.globl __mul16
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -14(ix)
	ld	d, -13(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, (_t)
	ld	e, -18(ix)
	ld	d, -17(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	inc	hl
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _go
	call	_go
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	.globl __mul16
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -30(ix)
	ld	d, -29(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	hl, (_t)
	ld	e, -34(ix)
	ld	d, -33(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	dec	hl
	ld	-38(ix), l
	ld	-37(ix), h
	jp	__xcc_L29
__xcc_L29:
__xcc_L25:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	jp	__xcc_L23
__xcc_L26:
	ld	hl, #0
	ld	6(ix), l
	ld	7(ix), h
__xcc_L21:
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	jp	__xcc_L19
__xcc_L22:
	ld	hl, #0
	jp	__go_end
__go_end:
	; epilogue: go
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #2
	push	hl
	ld	hl, #64
	push	hl
	.globl _calloc
	call	_calloc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	(_t), hl
	ld	hl, #0
	push	hl
	ld	hl, #0
	push	hl
	ld	hl, #0
	push	hl
	.globl _go
	call	_go
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, (_N)
	push	hl
	ld	hl, #92
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22862
	ld	hl, #0
	jp	__cmp_e_65123
__cmp_t_22862:
	ld	hl, #1
__cmp_e_65123:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L30
	jp	__xcc_L32
__xcc_L30:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L32
__xcc_L32:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
