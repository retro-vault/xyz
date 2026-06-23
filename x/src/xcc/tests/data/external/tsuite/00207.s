	.module xcc_output

	.area _CONST
__str_3:
	.db 98, 111, 111, 109, 33, 10, 0
__str_16:
	.db 37, 100, 10, 0
__str_20:
	.db 120, 49, 10, 0
__str_21:
	.db 37, 100, 10, 0
__str_25:
	.db 120, 50, 10, 0
__str_26:
	.db 37, 100, 10, 0
__str_30:
	.db 120, 51, 10, 0
__str_31:
	.db 37, 100, 10, 0
__str_35:
	.db 120, 52, 10, 0


	.area _CODE

	.globl _f1
_f1:
	; prologue: f1 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive param argc at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
label:
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__xcc_L2
__xcc_L2:
	ld	l, 4(ix)
	ld	h, 5(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	dec	hl
	ld	4(ix), l
	ld	5(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
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
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L6
__xcc_L4:
	jp	__f1_end
	jp	__xcc_L6
__xcc_L6:
	jp	label
__f1_end:
	; epilogue: f1
	ld	sp, ix
	pop	ix
	ret
	.globl _f2
_f2:
	; prologue: f2 (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	jp	start
	ld	hl, #1
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
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L8
	jp	__xcc_L7
__xcc_L8:
	ld	hl, #1
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
	ld	-16(ix), l
	ld	-15(ix), h
	jp	__xcc_L9
__xcc_L7:
	ld	hl, #0
	ld	-16(ix), l
	ld	-15(ix), h
__xcc_L9:
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
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
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
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	jp	__xcc_L11
__xcc_L11:
	ld	hl, #1
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
	ld	-24(ix), l
	ld	-23(ix), h
	jp	__xcc_L12
__xcc_L10:
	ld	hl, #1
	ld	-24(ix), l
	ld	-23(ix), h
__xcc_L12:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	jp	__xcc_L14
__xcc_L13:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	jp	__xcc_L15
__xcc_L14:
	ld	hl, #1
	ld	-30(ix), l
	ld	-29(ix), h
__xcc_L15:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	-12(ix), l
	ld	-11(ix), h
start:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, -36(ix)
	ld	d, -35(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
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
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, -44(ix)
	ld	d, -43(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__f2_end:
	; epilogue: f2
	ld	sp, ix
	pop	ix
	ret
	.globl _f3
_f3:
	; prologue: f3 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_16
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L17
	jp	__xcc_L18
__xcc_L17:
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L19
__xcc_L18:
	ld	hl, #11
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L19:
	ld	l, -8(ix)
	ld	h, -7(ix)
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
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #__str_21
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L22
	jp	__xcc_L23
__xcc_L22:
	ld	hl, #12
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	jp	__xcc_L24
__xcc_L23:
	ld	hl, #__str_25
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L24:
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #__str_26
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #0
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
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L28
	jp	__xcc_L27
__xcc_L28:
	ld	hl, #__str_30
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
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
	jp	nz, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	jp	__xcc_L29
__xcc_L27:
	ld	hl, #0
	ld	-30(ix), l
	ld	-29(ix), h
__xcc_L29:
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, #__str_31
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	hl, #1
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
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L32
	jp	__xcc_L33
__xcc_L33:
	ld	hl, #__str_35
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
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
	jp	nz, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	jp	__xcc_L34
__xcc_L32:
	ld	hl, #1
	ld	-42(ix), l
	ld	-41(ix), h
__xcc_L34:
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
__f3_end:
	; epilogue: f3
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
	.globl _f1
	call	_f1
	pop	bc
	.globl _f2
	call	_f2
	.globl _f3
	call	_f3
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
