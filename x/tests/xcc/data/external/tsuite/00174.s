	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 102, 10, 0
__str_1:
	.db 37, 102, 10, 0
__str_2:
	.db 37, 102, 10, 0
__str_3:
	.db 37, 102, 10, 0
__str_4:
	.db 37, 102, 10, 0
__str_5:
	.db 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 10, 0
__str_6:
	.db 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 10, 0
__str_7:
	.db 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 10, 0
__str_8:
	.db 37, 102, 10, 0
__str_9:
	.db 37, 102, 10, 0
__str_10:
	.db 37, 102, 10, 0
__str_11:
	.db 37, 102, 10, 0
__str_12:
	.db 37, 102, 10, 0
__str_13:
	.db 37, 102, 10, 0
__str_14:
	.db 37, 102, 10, 0
__str_15:
	.db 37, 102, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	.globl __fsadd
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	push	de
	pop	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
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
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	.globl __fsadd
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsadd
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	push	de
	pop	hl
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	.globl __fssub
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fssub
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
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	push	de
	pop	hl
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	.globl __fsdiv
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsdiv
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	push	de
	pop	hl
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	push	hl
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
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
	ld	-48(ix), l
	ld	-47(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_92777
	jp	m, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
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
	ld	-52(ix), l
	ld	-51(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_41421
	jp	p, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
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
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	l, -56(ix)
	ld	h, -55(ix)
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
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
	ld	-64(ix), l
	ld	-63(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_97763
	jp	m, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
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
	ld	-68(ix), l
	ld	-67(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_89172
	ld	hl, #0
	jp	__cmp_e_55736
__cmp_t_89172:
	ld	hl, #1
__cmp_e_55736:
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_95368
	jp	p, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
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
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65782
	ld	hl, #0
	jp	__cmp_e_21530
__cmp_t_65782:
	ld	hl, #1
__cmp_e_21530:
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_22862
	jp	m, __cmp_t_22862
	ld	hl, #0
	jp	__cmp_e_65123
__cmp_t_22862:
	ld	hl, #1
__cmp_e_65123:
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
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
	ld	-84(ix), l
	ld	-83(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_13929
	ld	hl, #0
	jp	__cmp_e_79802
__cmp_t_13929:
	ld	hl, #1
__cmp_e_79802:
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_23058
	jp	p, __cmp_t_34022
	ld	hl, #0
	jp	__cmp_e_23058
__cmp_t_34022:
	ld	hl, #1
__cmp_e_23058:
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	push	hl
	; load_rr: unhandled operand_kind 4
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
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -90(ix)
	ld	h, -89(ix)
	push	hl
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	ld	l, -86(ix)
	ld	h, -85(ix)
	push	hl
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	pop	de
	add	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	pop	de
	adc	hl, de
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -104(ix)
	ld	h, -103(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __mul32
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul32
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ex	de, hl
	push	de
	pop	hl
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -112(ix)
	ld	h, -111(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -110(ix)
	ld	h, -109(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __sdiv32
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__sdiv32
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	push	de
	pop	hl
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	hl, #__str_13
	dec	sp
	dec	sp
	ld	-130(ix), l
	ld	-129(ix), h
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
	ld	l, -132(ix)
	ld	h, -131(ix)
	push	hl
	ld	l, -130(ix)
	ld	h, -129(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-134(ix), l
	ld	-133(ix), h
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-136(ix), l
	ld	-135(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -136(ix)
	ld	h, -135(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-138(ix), l
	ld	-137(ix), h
	ld	hl, #__str_15
	dec	sp
	dec	sp
	ld	-140(ix), l
	ld	-139(ix), h
	ld	hl, #2
	push	hl
	.globl _sin
	call	_sin
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-144(ix), l
	ld	-143(ix), h
	push	de
	pop	hl
	ld	-142(ix), l
	ld	-141(ix), h
	ld	l, -142(ix)
	ld	h, -141(ix)
	push	hl
	ld	l, -144(ix)
	ld	h, -143(ix)
	push	hl
	ld	l, -140(ix)
	ld	h, -139(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-146(ix), l
	ld	-145(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
