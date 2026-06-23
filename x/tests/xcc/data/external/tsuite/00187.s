	.module xcc_output

	.area _CONST
__str_0:
	.db 102, 114, 101, 100, 46, 116, 120, 116, 0
__str_1:
	.db 119, 0
__str_2:
	.db 104, 101, 108, 108, 111, 10, 104, 101, 108, 108, 111, 10, 0
__str_3:
	.db 102, 114, 101, 100, 46, 116, 120, 116, 0
__str_4:
	.db 114, 0
__str_8:
	.db 99, 111, 117, 108, 100, 110, 39, 116, 32, 114, 101, 97, 100, 32, 102, 114, 101, 100, 46, 116, 120, 116, 10, 0
__str_9:
	.db 37, 115, 0
__str_10:
	.db 102, 114, 101, 100, 46, 116, 120, 116, 0
__str_11:
	.db 114, 0
__str_18:
	.db 99, 104, 58, 32, 37, 100, 32, 39, 37, 99, 39, 10, 0
__str_19:
	.db 102, 114, 101, 100, 46, 116, 120, 116, 0
__str_20:
	.db 114, 0
__str_27:
	.db 99, 104, 58, 32, 37, 100, 32, 39, 37, 99, 39, 10, 0
__str_28:
	.db 102, 114, 101, 100, 46, 116, 120, 116, 0
__str_29:
	.db 114, 0
__str_33:
	.db 120, 58, 32, 37, 115, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _fopen
	call	_fopen
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #12
	push	hl
	ld	hl, #1
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _fwrite
	call	_fwrite
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fclose
	call	_fclose
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _fopen
	call	_fopen
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #6
	push	hl
	ld	hl, #1
	push	hl
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	.globl _fread
	call	_fread
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #6
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
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	jp	__xcc_L7
__xcc_L5:
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	jp	__xcc_L7
__xcc_L7:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #6
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, -40(ix)
	ld	d, -39(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fclose
	call	_fclose
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
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
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	.globl _fopen
	call	_fopen
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L12:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fgetc
	call	_fgetc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	ld	-11(ix), l
	ld	-10(ix), h
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
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -11(ix)
	ld	h, -10(ix)
	push	hl
	ld	l, -58(ix)
	ld	h, -57(ix)
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
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	jp	__xcc_L14
__xcc_L13:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #32
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
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L17
__xcc_L15:
	ld	hl, #46
	ld	-12(ix), l
	ld	-11(ix), h
	jp	__xcc_L17
__xcc_L17:
	ld	hl, #__str_18
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	a, -12(ix)
	ld	l, a
	ld	h, #0
	push	hl
	ld	l, -11(ix)
	ld	h, -10(ix)
	push	hl
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	jp	__xcc_L12
__xcc_L14:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fclose
	call	_fclose
	pop	bc
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	hl, #__str_19
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	.globl _fopen
	call	_fopen
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L21:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _getc
	call	_getc
	pop	bc
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	ld	-11(ix), l
	ld	-10(ix), h
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
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -11(ix)
	ld	h, -10(ix)
	push	hl
	ld	l, -78(ix)
	ld	h, -77(ix)
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
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L22
	jp	__xcc_L23
__xcc_L22:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #32
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L24
	jp	__xcc_L26
__xcc_L24:
	ld	hl, #46
	ld	-12(ix), l
	ld	-11(ix), h
	jp	__xcc_L26
__xcc_L26:
	ld	hl, #__str_27
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	a, -12(ix)
	ld	l, a
	ld	h, #0
	push	hl
	ld	l, -11(ix)
	ld	h, -10(ix)
	push	hl
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	jp	__xcc_L21
__xcc_L23:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fclose
	call	_fclose
	pop	bc
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	hl, #__str_28
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	hl, #__str_29
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	push	hl
	ld	l, -90(ix)
	ld	h, -89(ix)
	push	hl
	.globl _fopen
	call	_fopen
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L30:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #7
	push	hl
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	.globl _fgets
	call	_fgets
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	push	hl
	ld	l, -98(ix)
	ld	h, -97(ix)
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
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L31
	jp	__xcc_L32
__xcc_L31:
	ld	hl, #__str_33
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	jp	__xcc_L30
__xcc_L32:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _fclose
	call	_fclose
	pop	bc
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
