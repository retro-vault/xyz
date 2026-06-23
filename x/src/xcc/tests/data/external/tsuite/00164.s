	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 10, 0
__str_1:
	.db 37, 100, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_9:
	.db 37, 100, 10, 0
__str_16:
	.db 37, 100, 10, 0
__str_23:
	.db 37, 100, 10, 0
__str_30:
	.db 37, 100, 10, 0
__str_34:
	.db 37, 100, 10, 0
__str_35:
	.db 37, 100, 44, 32, 37, 100, 10, 0
__str_36:
	.db 37, 100, 44, 32, 37, 100, 10, 0
__str_37:
	.db 37, 100, 10, 0
__str_41:
	.db 37, 100, 10, 0
__str_42:
	.db 37, 100, 10, 0
__str_43:
	.db 37, 100, 10, 0
__str_44:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=16)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-16
	add	hl, sp
	ld	sp, hl
	ld	hl, #12
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #34
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #56
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #78
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #1
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
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
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L4:
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
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
	jp	nz, __xcc_L7
	jp	__xcc_L6
__xcc_L7:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
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
	ld	-36(ix), l
	ld	-35(ix), h
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #0
	ld	-36(ix), l
	ld	-35(ix), h
__xcc_L8:
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	ld	hl, #0
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
	ld	-38(ix), l
	ld	-37(ix), h
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #1
	ld	-38(ix), l
	ld	-37(ix), h
__xcc_L5:
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
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
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	jp	__xcc_L11
__xcc_L11:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
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
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L14
	jp	__xcc_L13
__xcc_L14:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	jp	__xcc_L15
__xcc_L13:
	ld	hl, #0
	ld	-48(ix), l
	ld	-47(ix), h
__xcc_L15:
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	jp	__xcc_L12
__xcc_L10:
	ld	hl, #1
	ld	-50(ix), l
	ld	-49(ix), h
__xcc_L12:
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	hl, #__str_16
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L20
	jp	__xcc_L19
__xcc_L20:
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89172
	ld	hl, #0
	jp	__cmp_e_55736
__cmp_t_89172:
	ld	hl, #1
__cmp_e_55736:
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	jp	__xcc_L21
__xcc_L19:
	ld	hl, #0
	ld	-58(ix), l
	ld	-57(ix), h
__xcc_L21:
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L17
	jp	__xcc_L18
__xcc_L18:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2567
	ld	hl, #0
	jp	__cmp_e_56429
__cmp_t_2567:
	ld	hl, #1
__cmp_e_56429:
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	jp	__xcc_L22
__xcc_L17:
	ld	hl, #1
	ld	-62(ix), l
	ld	-61(ix), h
__xcc_L22:
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	hl, #__str_23
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65782
	ld	hl, #0
	jp	__cmp_e_21530
__cmp_t_65782:
	ld	hl, #1
__cmp_e_21530:
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L27
	jp	__xcc_L26
__xcc_L27:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
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
	ld	-70(ix), l
	ld	-69(ix), h
	jp	__xcc_L28
__xcc_L26:
	ld	hl, #0
	ld	-70(ix), l
	ld	-69(ix), h
__xcc_L28:
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74067
	ld	hl, #0
	jp	__cmp_e_3135
__cmp_t_74067:
	ld	hl, #1
__cmp_e_3135:
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -72(ix)
	ld	h, -71(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L24
	jp	__xcc_L25
__xcc_L25:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13929
	ld	hl, #0
	jp	__cmp_e_79802
__cmp_t_13929:
	ld	hl, #1
__cmp_e_79802:
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	jp	__xcc_L29
__xcc_L24:
	ld	hl, #1
	ld	-74(ix), l
	ld	-73(ix), h
__xcc_L29:
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, #__str_30
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34022
	ld	hl, #0
	jp	__cmp_e_23058
__cmp_t_34022:
	ld	hl, #1
__cmp_e_23058:
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L32
	jp	__xcc_L31
__xcc_L32:
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	pop	de
	ld	a, l
	or	a, e
	ld	l, a
	ld	a, h
	or	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33069
	ld	hl, #0
	jp	__cmp_e_98167
__cmp_t_33069:
	ld	hl, #1
__cmp_e_98167:
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	jp	__xcc_L33
__xcc_L31:
	ld	hl, #0
	ld	-84(ix), l
	ld	-83(ix), h
__xcc_L33:
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	hl, #__str_34
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	ld	a, l
	and	a, e
	ld	l, a
	ld	a, h
	and	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -90(ix)
	ld	h, -89(ix)
	pop	de
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -92(ix)
	ld	h, -91(ix)
	pop	de
	ld	a, l
	or	a, e
	ld	l, a
	ld	a, h
	or	a, d
	ld	h, a
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	push	hl
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	hl, #__str_35
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	ld	l, -98(ix)
	ld	h, -97(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	hl, #__str_36
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76229
	ld	hl, #0
	jp	__cmp_e_77373
__cmp_t_76229:
	ld	hl, #1
__cmp_e_77373:
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_84421
	ld	hl, #0
	jp	__cmp_e_44919
__cmp_t_84421:
	ld	hl, #1
__cmp_e_44919:
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -110(ix)
	ld	h, -109(ix)
	push	hl
	ld	l, -108(ix)
	ld	h, -107(ix)
	push	hl
	ld	l, -106(ix)
	ld	h, -105(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ld	hl, #__str_37
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13784
	ld	hl, #0
	jp	__cmp_e_98537
__cmp_t_13784:
	ld	hl, #1
__cmp_e_98537:
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	l, -116(ix)
	ld	h, -115(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75198
	ld	hl, #0
	jp	__cmp_e_94324
__cmp_t_75198:
	ld	hl, #1
__cmp_e_94324:
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L39
	jp	__xcc_L38
__xcc_L39:
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98315
	ld	hl, #0
	jp	__cmp_e_64370
__cmp_t_98315:
	ld	hl, #1
__cmp_e_64370:
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66413
	ld	hl, #0
	jp	__cmp_e_3526
__cmp_t_66413:
	ld	hl, #1
__cmp_e_3526:
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	jp	__xcc_L40
__xcc_L38:
	ld	hl, #0
	ld	-122(ix), l
	ld	-121(ix), h
__xcc_L40:
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	hl, #__str_41
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	.globl __mul16
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -128(ix)
	ld	h, -127(ix)
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-130(ix), l
	ld	-129(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -130(ix)
	ld	d, -129(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
	ld	l, -132(ix)
	ld	h, -131(ix)
	push	hl
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-134(ix), l
	ld	-133(ix), h
	ld	hl, #__str_42
	dec	sp
	dec	sp
	ld	-136(ix), l
	ld	-135(ix), h
	.globl __mul16
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-138(ix), l
	ld	-137(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -138(ix)
	ld	h, -137(ix)
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-140(ix), l
	ld	-139(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -140(ix)
	ld	d, -139(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-142(ix), l
	ld	-141(ix), h
	ld	l, -142(ix)
	ld	h, -141(ix)
	push	hl
	ld	l, -136(ix)
	ld	h, -135(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-144(ix), l
	ld	-143(ix), h
	ld	hl, #__str_43
	dec	sp
	dec	sp
	ld	-146(ix), l
	ld	-145(ix), h
	ld	hl, #4
	push	hl
	ld	hl, #4
	ld	b, l
	pop	hl
__shift_6091:
	ld	a, b
	or	a, a
	jp	z, __sdone_8980
	add	hl, hl
	djnz	__shift_6091
__sdone_8980:
	dec	sp
	dec	sp
	ld	-148(ix), l
	ld	-147(ix), h
	ld	l, -148(ix)
	ld	h, -147(ix)
	push	hl
	ld	l, -146(ix)
	ld	h, -145(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-150(ix), l
	ld	-149(ix), h
	ld	hl, #__str_44
	dec	sp
	dec	sp
	ld	-152(ix), l
	ld	-151(ix), h
	ld	hl, #64
	push	hl
	ld	hl, #4
	ld	b, l
	pop	hl
__shift_9956:
	ld	a, b
	or	a, a
	jp	z, __sdone_1873
	sra	h
	rr	l
	djnz	__shift_9956
__sdone_1873:
	dec	sp
	dec	sp
	ld	-154(ix), l
	ld	-153(ix), h
	ld	l, -154(ix)
	ld	h, -153(ix)
	push	hl
	ld	l, -152(ix)
	ld	h, -151(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-156(ix), l
	ld	-155(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
