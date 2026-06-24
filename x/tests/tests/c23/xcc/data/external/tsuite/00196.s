	.module xcc_output

	.area _CONST
__str_0:
	.db 102, 114, 101, 100, 10, 0
__str_1:
	.db 106, 111, 101, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_6:
	.db 37, 100, 10, 0
__str_10:
	.db 37, 100, 10, 0
__str_14:
	.db 37, 100, 10, 0
__str_18:
	.db 37, 100, 10, 0
__str_22:
	.db 37, 100, 10, 0
__str_26:
	.db 37, 100, 10, 0
__str_30:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _fred
_fred:
	; prologue: fred (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	jp	__fred_end
__fred_end:
	; epilogue: fred
	ld	sp, ix
	pop	ix
	ret
	.globl _joe
_joe:
	; prologue: joe (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	jp	__joe_end
__joe_end:
	; epilogue: joe
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L3
__xcc_L4:
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
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
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #0
	ld	-10(ix), l
	ld	-9(ix), h
__xcc_L5:
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
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
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L7
	jp	__xcc_L8
__xcc_L8:
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
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
	ld	-22(ix), l
	ld	-21(ix), h
	jp	__xcc_L9
__xcc_L7:
	ld	hl, #1
	ld	-22(ix), l
	ld	-21(ix), h
__xcc_L9:
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
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
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L11
__xcc_L12:
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
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
	ld	-34(ix), l
	ld	-33(ix), h
	jp	__xcc_L13
__xcc_L11:
	ld	hl, #0
	ld	-34(ix), l
	ld	-33(ix), h
__xcc_L13:
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
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
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L16
__xcc_L16:
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
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
	ld	-46(ix), l
	ld	-45(ix), h
	jp	__xcc_L17
__xcc_L15:
	ld	hl, #1
	ld	-46(ix), l
	ld	-45(ix), h
__xcc_L17:
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #__str_18
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
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
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L20
	jp	__xcc_L19
__xcc_L20:
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #1
	ld	e, -56(ix)
	ld	d, -55(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
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
	ld	-60(ix), l
	ld	-59(ix), h
	jp	__xcc_L21
__xcc_L19:
	ld	hl, #0
	ld	-60(ix), l
	ld	-59(ix), h
__xcc_L21:
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	hl, #__str_22
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
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
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L23
	jp	__xcc_L24
__xcc_L24:
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	hl, #0
	ld	e, -70(ix)
	ld	d, -69(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -72(ix)
	ld	h, -71(ix)
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
	ld	-74(ix), l
	ld	-73(ix), h
	jp	__xcc_L25
__xcc_L23:
	ld	hl, #1
	ld	-74(ix), l
	ld	-73(ix), h
__xcc_L25:
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, #__str_26
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
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
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L28
	jp	__xcc_L27
__xcc_L28:
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	hl, #0
	ld	e, -84(ix)
	ld	d, -83(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
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
	ld	-88(ix), l
	ld	-87(ix), h
	jp	__xcc_L29
__xcc_L27:
	ld	hl, #0
	ld	-88(ix), l
	ld	-87(ix), h
__xcc_L29:
	ld	l, -88(ix)
	ld	h, -87(ix)
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
	ld	-90(ix), l
	ld	-89(ix), h
	ld	hl, #__str_30
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	.globl _joe
	call	_joe
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
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
	ld	-96(ix), l
	ld	-95(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L31
	jp	__xcc_L32
__xcc_L32:
	.globl _fred
	call	_fred
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	hl, #1
	ld	e, -98(ix)
	ld	d, -97(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
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
	ld	-102(ix), l
	ld	-101(ix), h
	jp	__xcc_L33
__xcc_L31:
	ld	hl, #1
	ld	-102(ix), l
	ld	-101(ix), h
__xcc_L33:
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	ld	l, -92(ix)
	ld	h, -91(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
