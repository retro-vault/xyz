	.module xcc_output

	.area _CONST
__str_4:
	.db 37, 100, 58, 32, 37, 100, 10, 0
__str_9:
	.db 37, 100, 58, 32, 37, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=42)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-42
	add	hl, sp
	ld	sp, hl
	push	ix
	pop	hl
	ld	de, #-22
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	de, #12
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	de, #34
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	de, #56
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	de, #78
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #8
	add	hl, de
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	de, #90
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #10
	add	hl, de
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	de, #123
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #12
	add	hl, de
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	push	hl
	ld	de, #456
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #14
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	de, #789
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	de, #8642
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	de, #18
	add	hl, de
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	de, #9753
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L0:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #10
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
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
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
	ld	l, -22(ix)
	ld	h, -21(ix)
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
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
__xcc_L2:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L0
__xcc_L3:
	push	ix
	pop	hl
	ld	de, #-42
	add	hl, de
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	ld	de, #12
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	ld	de, #34
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	ld	de, #56
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	de, #78
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #8
	add	hl, de
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	push	hl
	ld	de, #90
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #10
	add	hl, de
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	ld	de, #123
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #12
	add	hl, de
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -90(ix)
	ld	h, -89(ix)
	push	hl
	ld	de, #456
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #14
	add	hl, de
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	push	hl
	ld	de, #789
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	push	hl
	ld	de, #8642
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -78(ix)
	ld	h, -77(ix)
	ld	de, #18
	add	hl, de
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	push	hl
	ld	de, #9753
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L5:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #10
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
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -98(ix)
	ld	h, -97(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
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
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
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
	ld	l, -106(ix)
	ld	h, -105(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -100(ix)
	ld	h, -99(ix)
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
__xcc_L7:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L5
__xcc_L8:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
