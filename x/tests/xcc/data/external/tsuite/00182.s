	.module xcc_output

	.area _DATA
_print_led__d_0:
	.ds 64

	.area _CONST
__str_82:
	.db 37, 115, 10, 0


	.area _CODE

	.globl _topline
_topline:
	; prologue: topline (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param d at 4(ix)
	; receive param p at 6(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L2
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #5
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #7
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #9
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L0:
__xcc_L1:
__xcc_L2:
__xcc_L3:
__xcc_L4:
__xcc_L5:
__xcc_L6:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	de, #95
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L8
__xcc_L7:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L8:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__topline_end:
	; epilogue: topline
	ld	sp, ix
	pop	ix
	ret
	.globl _midline
_midline:
	; prologue: midline (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param d at 4(ix)
	; receive param p at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #4
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #5
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L11
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #6
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_2567
	ld	hl, #0
	jp	__cmp_e_56429
__cmp_t_2567:
	ld	hl, #1
__cmp_e_56429:
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #9
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
	jp	nz, __xcc_L14
	jp	__xcc_L15
__xcc_L9:
__xcc_L10:
__xcc_L11:
__xcc_L12:
__xcc_L13:
__xcc_L14:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	de, #124
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L16
__xcc_L15:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L16:
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_22862
	ld	hl, #0
	jp	__cmp_e_65123
__cmp_t_22862:
	ld	hl, #1
__cmp_e_65123:
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L17
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_74067
	ld	hl, #0
	jp	__cmp_e_3135
__cmp_t_74067:
	ld	hl, #1
__cmp_e_3135:
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L18
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #4
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_13929
	ld	hl, #0
	jp	__cmp_e_79802
__cmp_t_13929:
	ld	hl, #1
__cmp_e_79802:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L19
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #5
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_34022
	ld	hl, #0
	jp	__cmp_e_23058
__cmp_t_34022:
	ld	hl, #1
__cmp_e_23058:
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L20
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #6
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_33069
	ld	hl, #0
	jp	__cmp_e_98167
__cmp_t_33069:
	ld	hl, #1
__cmp_e_98167:
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L21
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_61393
	ld	hl, #0
	jp	__cmp_e_18456
__cmp_t_61393:
	ld	hl, #1
__cmp_e_18456:
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L22
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #9
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_75011
	ld	hl, #0
	jp	__cmp_e_78042
__cmp_t_75011:
	ld	hl, #1
__cmp_e_78042:
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L23
	jp	__xcc_L24
__xcc_L17:
__xcc_L18:
__xcc_L19:
__xcc_L20:
__xcc_L21:
__xcc_L22:
__xcc_L23:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	de, #95
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L25
__xcc_L24:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L25:
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_76229
	ld	hl, #0
	jp	__cmp_e_77373
__cmp_t_76229:
	ld	hl, #1
__cmp_e_77373:
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L26
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_84421
	ld	hl, #0
	jp	__cmp_e_44919
__cmp_t_84421:
	ld	hl, #1
__cmp_e_44919:
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L27
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_13784
	ld	hl, #0
	jp	__cmp_e_98537
__cmp_t_13784:
	ld	hl, #1
__cmp_e_98537:
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L28
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_75198
	ld	hl, #0
	jp	__cmp_e_94324
__cmp_t_75198:
	ld	hl, #1
__cmp_e_94324:
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L29
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #4
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_98315
	ld	hl, #0
	jp	__cmp_e_64370
__cmp_t_98315:
	ld	hl, #1
__cmp_e_64370:
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L30
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #7
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_66413
	ld	hl, #0
	jp	__cmp_e_3526
__cmp_t_66413:
	ld	hl, #1
__cmp_e_3526:
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L31
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_76091
	ld	hl, #0
	jp	__cmp_e_68980
__cmp_t_76091:
	ld	hl, #1
__cmp_e_68980:
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L32
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #9
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_59956
	ld	hl, #0
	jp	__cmp_e_41873
__cmp_t_59956:
	ld	hl, #1
__cmp_e_41873:
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L33
	jp	__xcc_L34
__xcc_L26:
__xcc_L27:
__xcc_L28:
__xcc_L29:
__xcc_L30:
__xcc_L31:
__xcc_L32:
__xcc_L33:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	de, #124
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L35
__xcc_L34:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L35:
__midline_end:
	; epilogue: midline
	ld	sp, ix
	pop	ix
	ret
	.globl _botline
_botline:
	; prologue: botline (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param d at 4(ix)
	; receive param p at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_6862
	ld	hl, #0
	jp	__cmp_e_99170
__cmp_t_6862:
	ld	hl, #1
__cmp_e_99170:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L36
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_6996
	ld	hl, #0
	jp	__cmp_e_97281
__cmp_t_6996:
	ld	hl, #1
__cmp_e_97281:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L37
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #6
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_2305
	ld	hl, #0
	jp	__cmp_e_20925
__cmp_t_2305:
	ld	hl, #1
__cmp_e_20925:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L38
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_77084
	ld	hl, #0
	jp	__cmp_e_36327
__cmp_t_77084:
	ld	hl, #1
__cmp_e_36327:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L39
	jp	__xcc_L40
__xcc_L36:
__xcc_L37:
__xcc_L38:
__xcc_L39:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	de, #124
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L41
__xcc_L40:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L41:
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_60336
	ld	hl, #0
	jp	__cmp_e_26505
__cmp_t_60336:
	ld	hl, #1
__cmp_e_26505:
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L42
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_50846
	ld	hl, #0
	jp	__cmp_e_21729
__cmp_t_50846:
	ld	hl, #1
__cmp_e_21729:
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L43
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_61313
	ld	hl, #0
	jp	__cmp_e_25857
__cmp_t_61313:
	ld	hl, #1
__cmp_e_25857:
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L44
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #5
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_16124
	ld	hl, #0
	jp	__cmp_e_53895
__cmp_t_16124:
	ld	hl, #1
__cmp_e_53895:
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L45
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #6
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_19582
	ld	hl, #0
	jp	__cmp_e_545
__cmp_t_19582:
	ld	hl, #1
__cmp_e_545:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L46
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_98814
	ld	hl, #0
	jp	__cmp_e_33367
__cmp_t_98814:
	ld	hl, #1
__cmp_e_33367:
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L47
	jp	__xcc_L48
__xcc_L42:
__xcc_L43:
__xcc_L44:
__xcc_L45:
__xcc_L46:
__xcc_L47:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	de, #95
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L49
__xcc_L48:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L49:
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_15434
	ld	hl, #0
	jp	__cmp_e_90364
__cmp_t_15434:
	ld	hl, #1
__cmp_e_90364:
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L50
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_44043
	ld	hl, #0
	jp	__cmp_e_13750
__cmp_t_44043:
	ld	hl, #1
__cmp_e_13750:
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L51
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_71087
	ld	hl, #0
	jp	__cmp_e_26808
__cmp_t_71087:
	ld	hl, #1
__cmp_e_26808:
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L52
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #4
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_17276
	ld	hl, #0
	jp	__cmp_e_47178
__cmp_t_17276:
	ld	hl, #1
__cmp_e_47178:
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L53
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #5
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_95788
	ld	hl, #0
	jp	__cmp_e_93584
__cmp_t_95788:
	ld	hl, #1
__cmp_e_93584:
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L54
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #6
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_5403
	ld	hl, #0
	jp	__cmp_e_2651
__cmp_t_5403:
	ld	hl, #1
__cmp_e_2651:
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L55
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #7
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_92754
	ld	hl, #0
	jp	__cmp_e_12399
__cmp_t_92754:
	ld	hl, #1
__cmp_e_12399:
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L56
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #8
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_99932
	ld	hl, #0
	jp	__cmp_e_95060
__cmp_t_99932:
	ld	hl, #1
__cmp_e_95060:
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L57
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #9
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_49676
	ld	hl, #0
	jp	__cmp_e_93368
__cmp_t_49676:
	ld	hl, #1
__cmp_e_93368:
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L58
	jp	__xcc_L59
__xcc_L50:
__xcc_L51:
__xcc_L52:
__xcc_L53:
__xcc_L54:
__xcc_L55:
__xcc_L56:
__xcc_L57:
__xcc_L58:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	de, #124
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jp	__xcc_L60
__xcc_L59:
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L60:
__botline_end:
	; epilogue: botline
	ld	sp, ix
	pop	ix
	ret
	.globl _print_led
_print_led:
	; prologue: print_led (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive param x at 4(ix)
	; receive param buf at 8(ix)
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_47739
	ld	hl, #0
	jp	__cmp_e_10012
__cmp_t_47739:
	ld	hl, #1
__cmp_e_10012:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L61
	jp	__xcc_L62
__xcc_L61:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L63
__xcc_L62:
	ld	hl, #0
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L63:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L64:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L65
	jp	__xcc_L66
__xcc_L65:
	.globl __mod32
	ld	hl, #0
	push	hl
	ld	hl, #10
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__mod32
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	push	de
	pop	hl
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
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
	ld	hl, (_print_led__d_0)
	ld	e, -18(ix)
	ld	d, -17(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	e, -14(ix)
	ld	d, -13(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #32
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_36226
	ld	hl, #0
	jp	__cmp_e_98586
__cmp_t_36226:
	ld	hl, #1
__cmp_e_98586:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L67
	jp	__xcc_L69
__xcc_L67:
	jp	__xcc_L66
	jp	__xcc_L69
__xcc_L69:
	.globl __div32
	ld	hl, #0
	push	hl
	ld	hl, #10
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__div32
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	push	de
	pop	hl
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	4(ix), l
	ld	5(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	6(ix), l
	ld	7(ix), h
	jp	__xcc_L64
__xcc_L66:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L70:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_48094
	ld	hl, #0
	jp	__cmp_e_97539
__cmp_t_48094:
	ld	hl, #1
__cmp_e_97539:
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L71
	jp	__xcc_L73
__xcc_L71:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, (_print_led__d_0)
	ld	e, -32(ix)
	ld	d, -31(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	.globl _topline
	call	_topline
	pop	bc
	pop	bc
	ld	l, 8(ix)
	ld	h, 9(ix)
	ld	de, #3
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	8(ix), l
	ld	9(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L72:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L70
__xcc_L73:
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
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	de, #10
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L74:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_40795
	ld	hl, #0
	jp	__cmp_e_80570
__cmp_t_40795:
	ld	hl, #1
__cmp_e_80570:
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L75
	jp	__xcc_L77
__xcc_L75:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, (_print_led__d_0)
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
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	.globl _midline
	call	_midline
	pop	bc
	pop	bc
	ld	l, 8(ix)
	ld	h, 9(ix)
	ld	de, #3
	add	hl, de
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	ld	8(ix), l
	ld	9(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L76:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L74
__xcc_L77:
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	de, #10
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L78:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_51434
	ld	hl, #0
	jp	__cmp_e_60378
__cmp_t_51434:
	ld	hl, #1
__cmp_e_60378:
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L79
	jp	__xcc_L81
__xcc_L79:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	hl, (_print_led__d_0)
	ld	e, -68(ix)
	ld	d, -67(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
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
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	.globl _botline
	call	_botline
	pop	bc
	pop	bc
	ld	l, 8(ix)
	ld	h, 9(ix)
	ld	de, #3
	add	hl, de
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	ld	8(ix), l
	ld	9(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	ld	de, #32
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L80:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L78
__xcc_L81:
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	inc	hl
	ld	8(ix), l
	ld	9(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	ld	de, #10
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__print_led_end:
	; epilogue: print_led
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	.globl __mul16
	ld	hl, #32
	push	hl
	ld	hl, #5
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #18
	push	hl
	ld	hl, #54919
	push	hl
	.globl _print_led
	call	_print_led
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_82
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
