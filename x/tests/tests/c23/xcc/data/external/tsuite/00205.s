	.module xcc_output

	.area _DATA
	.globl _cases
_cases:
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28
	.ds 28

	.area _CONST
__str_8:
	.db 99, 97, 115, 101, 115, 91, 37, 100, 93, 46, 99, 91, 37, 100, 93, 61, 37, 108, 100, 10, 0
__str_9:
	.db 99, 97, 115, 101, 115, 91, 37, 100, 93, 46, 98, 61, 37, 108, 100, 10, 0
__str_10:
	.db 99, 97, 115, 101, 115, 91, 37, 100, 93, 46, 101, 61, 37, 108, 100, 10, 0
__str_11:
	.db 99, 97, 115, 101, 115, 91, 37, 100, 93, 46, 107, 61, 37, 108, 100, 10, 0
__str_12:
	.db 10, 0


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
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L0:
	ld	hl, #0
	push	hl
	ld	hl, #0
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L4:
	ld	hl, #0
	push	hl
	ld	hl, #0
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	jp	__xcc_L7
__xcc_L5:
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, (_cases)
	ld	e, -16(ix)
	ld	d, -15(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
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
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
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
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	e, -26(ix)
	ld	d, -25(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
__xcc_L6:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L4
__xcc_L7:
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	hl, (_cases)
	ld	e, -38(ix)
	ld	d, -37(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	push	ix
	pop	hl
	ld	de, #-42
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
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
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	.globl _printf
	call	_printf
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
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	hl, (_cases)
	ld	e, -52(ix)
	ld	d, -51(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	push	ix
	pop	hl
	ld	de, #-56
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
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
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
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
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	hl, (_cases)
	ld	e, -66(ix)
	ld	d, -65(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	push	ix
	pop	hl
	ld	de, #-70
	add	hl, de
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -72(ix)
	ld	h, -71(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-76(ix), l
	ld	-75(ix), h
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
__xcc_L2:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
