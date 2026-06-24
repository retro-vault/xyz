	.module xcc_output

	.area _CONST
__str_0:
	.db 104, 101, 108, 108, 111, 0
__str_1:
	.db 37, 115, 10, 0
__str_2:
	.db 103, 111, 115, 104, 0
__str_3:
	.db 37, 115, 10, 0
__str_4:
	.db 37, 100, 10, 0
__str_5:
	.db 97, 112, 112, 108, 101, 0
__str_6:
	.db 37, 100, 10, 0
__str_7:
	.db 103, 111, 101, 114, 101, 0
__str_8:
	.db 37, 100, 10, 0
__str_9:
	.db 122, 101, 98, 114, 97, 0
__str_10:
	.db 37, 100, 10, 0
__str_11:
	.db 33, 0
__str_12:
	.db 37, 115, 10, 0
__str_13:
	.db 37, 100, 10, 0
__str_14:
	.db 97, 112, 112, 108, 101, 0
__str_15:
	.db 37, 100, 10, 0
__str_16:
	.db 103, 111, 101, 114, 101, 0
__str_17:
	.db 37, 100, 10, 0
__str_18:
	.db 103, 111, 101, 114, 103, 0
__str_19:
	.db 37, 100, 10, 0
__str_20:
	.db 122, 101, 98, 114, 97, 0
__str_21:
	.db 37, 115, 10, 0
__str_22:
	.db 37, 115, 10, 0
__str_23:
	.db 37, 100, 10, 0
__str_24:
	.db 37, 115, 10, 0
__str_25:
	.db 37, 115, 10, 0
__str_26:
	.db 37, 100, 10, 0
__str_27:
	.db 97, 112, 112, 108, 101, 0
__str_28:
	.db 37, 100, 10, 0
__str_29:
	.db 103, 114, 103, 114, 0
__str_30:
	.db 37, 100, 10, 0
__str_31:
	.db 122, 101, 98, 114, 97, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=10)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strcpy
	call	_strcpy
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #2
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strncpy
	call	_strncpy
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strcmp
	call	_strcmp
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
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_30886
	jp	p, __cmp_t_89383
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
	push	hl
	ld	l, -28(ix)
	ld	h, -27(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strcmp
	call	_strcmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_36915
	jp	p, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
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
	ld	-46(ix), l
	ld	-45(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strcmp
	call	_strcmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
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
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strlen
	call	_strlen
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strcat
	call	_strcat
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	hl, #__str_13
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	hl, #2
	push	hl
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strncmp
	call	_strncmp
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
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_60492
	jp	p, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	hl, #__str_15
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	hl, #__str_16
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	hl, #2
	push	hl
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strncmp
	call	_strncmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
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
	jp	z, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	hl, #__str_17
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	hl, #__str_18
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	hl, #2
	push	hl
	ld	l, -94(ix)
	ld	h, -93(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strncmp
	call	_strncmp
	pop	bc
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
	ld	l, -96(ix)
	ld	h, -95(ix)
	push	hl
	ld	hl, #0
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
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -98(ix)
	ld	h, -97(ix)
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
	ld	-100(ix), l
	ld	-99(ix), h
	ld	hl, #__str_19
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	hl, #2
	push	hl
	ld	l, -104(ix)
	ld	h, -103(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strncmp
	call	_strncmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -106(ix)
	ld	h, -105(ix)
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
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, -108(ix)
	ld	h, -107(ix)
	push	hl
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	hl, #__str_21
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ld	hl, #111
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strchr
	call	_strchr
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	ld	l, -112(ix)
	ld	h, -111(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	hl, #__str_22
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	hl, #108
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strrchr
	call	_strrchr
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	push	hl
	ld	l, -118(ix)
	ld	h, -117(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	hl, #__str_23
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	hl, #120
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _strrchr
	call	_strrchr
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	ld	l, -128(ix)
	ld	h, -127(ix)
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
	ld	-130(ix), l
	ld	-129(ix), h
	ld	l, -130(ix)
	ld	h, -129(ix)
	push	hl
	ld	l, -124(ix)
	ld	h, -123(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
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
	ld	-134(ix), l
	ld	-133(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, -134(ix)
	ld	d, -133(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-136(ix), l
	ld	-135(ix), h
	ld	l, -136(ix)
	ld	h, -135(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-138(ix), l
	ld	-137(ix), h
	push	ix
	pop	hl
	ld	de, #-138
	add	hl, de
	dec	sp
	dec	sp
	ld	-140(ix), l
	ld	-139(ix), h
	ld	hl, #4
	push	hl
	ld	hl, #114
	push	hl
	ld	l, -140(ix)
	ld	h, -139(ix)
	push	hl
	.globl _memset
	call	_memset
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-142(ix), l
	ld	-141(ix), h
	ld	hl, #__str_24
	dec	sp
	dec	sp
	ld	-144(ix), l
	ld	-143(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -144(ix)
	ld	h, -143(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-146(ix), l
	ld	-145(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #2
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-148(ix), l
	ld	-147(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, -148(ix)
	ld	d, -147(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-150(ix), l
	ld	-149(ix), h
	ld	l, -150(ix)
	ld	h, -149(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-152(ix), l
	ld	-151(ix), h
	push	ix
	pop	hl
	ld	de, #-152
	add	hl, de
	dec	sp
	dec	sp
	ld	-154(ix), l
	ld	-153(ix), h
	ld	hl, #2
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -154(ix)
	ld	h, -153(ix)
	push	hl
	.globl _memcpy
	call	_memcpy
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-156(ix), l
	ld	-155(ix), h
	ld	hl, #__str_25
	dec	sp
	dec	sp
	ld	-158(ix), l
	ld	-157(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -158(ix)
	ld	h, -157(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-160(ix), l
	ld	-159(ix), h
	ld	hl, #__str_26
	dec	sp
	dec	sp
	ld	-162(ix), l
	ld	-161(ix), h
	ld	hl, #__str_27
	dec	sp
	dec	sp
	ld	-164(ix), l
	ld	-163(ix), h
	ld	hl, #4
	push	hl
	ld	l, -164(ix)
	ld	h, -163(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _memcmp
	call	_memcmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-166(ix), l
	ld	-165(ix), h
	ld	l, -166(ix)
	ld	h, -165(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_83426
	jp	p, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-168(ix), l
	ld	-167(ix), h
	ld	l, -168(ix)
	ld	h, -167(ix)
	push	hl
	ld	l, -162(ix)
	ld	h, -161(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-170(ix), l
	ld	-169(ix), h
	ld	hl, #__str_28
	dec	sp
	dec	sp
	ld	-172(ix), l
	ld	-171(ix), h
	ld	hl, #__str_29
	dec	sp
	dec	sp
	ld	-174(ix), l
	ld	-173(ix), h
	ld	hl, #4
	push	hl
	ld	l, -174(ix)
	ld	h, -173(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _memcmp
	call	_memcmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-176(ix), l
	ld	-175(ix), h
	ld	l, -176(ix)
	ld	h, -175(ix)
	push	hl
	ld	hl, #0
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
	ld	-178(ix), l
	ld	-177(ix), h
	ld	l, -178(ix)
	ld	h, -177(ix)
	push	hl
	ld	l, -172(ix)
	ld	h, -171(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-180(ix), l
	ld	-179(ix), h
	ld	hl, #__str_30
	dec	sp
	dec	sp
	ld	-182(ix), l
	ld	-181(ix), h
	ld	hl, #__str_31
	dec	sp
	dec	sp
	ld	-184(ix), l
	ld	-183(ix), h
	ld	hl, #4
	push	hl
	ld	l, -184(ix)
	ld	h, -183(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _memcmp
	call	_memcmp
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-186(ix), l
	ld	-185(ix), h
	ld	l, -186(ix)
	ld	h, -185(ix)
	push	hl
	ld	hl, #0
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
	ld	-188(ix), l
	ld	-187(ix), h
	ld	l, -188(ix)
	ld	h, -187(ix)
	push	hl
	ld	l, -182(ix)
	ld	h, -181(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-190(ix), l
	ld	-189(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
