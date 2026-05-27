	.module xcc_output

	.area _DATA
	.globl _A
_A:
	.ds 8
	.globl _B
_B:
	.ds 8
	.globl _C
_C:
	.ds 8

	.area _CONST
__str_0:
	.db 65, 58, 32, 0
__str_5:
	.db 32, 37, 100, 32, 0
__str_6:
	.db 10, 0
__str_7:
	.db 66, 58, 32, 0
__str_12:
	.db 32, 37, 100, 32, 0
__str_13:
	.db 10, 0
__str_14:
	.db 67, 58, 32, 0
__str_19:
	.db 32, 37, 100, 32, 0
__str_20:
	.db 10, 0
__str_21:
	.db 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 0
__str_49:
	.db 83, 111, 108, 117, 116, 105, 111, 110, 32, 111, 102, 32, 84, 111, 119, 101, 114, 32, 111, 102, 32, 72, 97, 110, 111, 105, 32, 80, 114, 111, 98, 108, 101, 109, 32, 119, 105, 116, 104, 32, 37, 100, 32, 68, 105, 115, 107, 115, 10, 10, 0
__str_50:
	.db 83, 116, 97, 114, 116, 105, 110, 103, 32, 115, 116, 97, 116, 101, 58, 10, 0
__str_51:
	.db 10, 10, 83, 117, 98, 115, 101, 113, 117, 101, 110, 116, 32, 115, 116, 97, 116, 101, 115, 58, 10, 10, 0


	.area _CODE

	.globl _PrintAll
_PrintAll:
	; prologue: PrintAll (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
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
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L1:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
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
	jp	nz, __xcc_L2
	jp	__xcc_L4
__xcc_L2:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, (_A)
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
	ld	l, -16(ix)
	ld	h, -15(ix)
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
	ld	-18(ix), l
	ld	-17(ix), h
__xcc_L3:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L1
__xcc_L4:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #__str_7
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
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L8:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
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
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L11
__xcc_L9:
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
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
	ld	-34(ix), l
	ld	-33(ix), h
	ld	hl, (_B)
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
	push	hl
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
__xcc_L10:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L8
__xcc_L11:
	ld	hl, #__str_13
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L15:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
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
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L16
	jp	__xcc_L18
__xcc_L16:
	ld	hl, #__str_19
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
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
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, (_C)
	ld	e, -56(ix)
	ld	d, -55(ix)
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
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
__xcc_L17:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L15
__xcc_L18:
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	hl, #__str_21
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	jp	__PrintAll_end
__PrintAll_end:
	; epilogue: PrintAll
	ld	sp, ix
	pop	ix
	ret
	.globl _Move
_Move:
	; prologue: Move (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive param source at 4(ix)
	; receive param dest at 6(ix)
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L22:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L26
	jp	__xcc_L25
__xcc_L26:
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
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -10(ix)
	ld	d, -9(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
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
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
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
	ld	-18(ix), l
	ld	-17(ix), h
	jp	__xcc_L27
__xcc_L25:
	ld	hl, #0
	ld	-18(ix), l
	ld	-17(ix), h
__xcc_L27:
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L23
	jp	__xcc_L24
__xcc_L23:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L22
__xcc_L24:
__xcc_L28:
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #4
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
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
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L32
	jp	__xcc_L31
__xcc_L32:
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
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
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
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
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
	ld	-34(ix), l
	ld	-33(ix), h
	jp	__xcc_L33
__xcc_L31:
	ld	hl, #0
	ld	-34(ix), l
	ld	-33(ix), h
__xcc_L33:
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L29
	jp	__xcc_L30
__xcc_L29:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L28
__xcc_L30:
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
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
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
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -46(ix)
	ld	d, -45(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	e, -42(ix)
	ld	d, -41(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
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
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -50(ix)
	ld	d, -49(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl _PrintAll
	call	_PrintAll
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	e, -56(ix)
	ld	d, -55(ix)
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
	jp	__Move_end
__Move_end:
	; epilogue: Move
	ld	sp, ix
	pop	ix
	ret
	.globl _Hanoi
_Hanoi:
	; prologue: Hanoi (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	; receive param n at 4(ix)
	; receive param source at 6(ix)
	; receive param dest at 8(ix)
	; receive param spare at 10(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L34
	jp	__xcc_L36
__xcc_L34:
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	.globl _Move
	call	_Move
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	jp	__Hanoi_end
	jp	__xcc_L36
__xcc_L36:
	ld	l, 4(ix)
	ld	h, 5(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 10(ix)
	ld	h, 11(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _Hanoi
	call	_Hanoi
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	.globl _Move
	call	_Move
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 10(ix)
	ld	h, 11(ix)
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	.globl _Hanoi
	call	_Hanoi
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	jp	__Hanoi_end
__Hanoi_end:
	; epilogue: Hanoi
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L37:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L38
	jp	__xcc_L40
__xcc_L38:
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, (_A)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	e, -6(ix)
	ld	d, -5(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L39:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L37
__xcc_L40:
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L41:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22862
	ld	hl, #0
	jp	__cmp_e_65123
__cmp_t_22862:
	ld	hl, #1
__cmp_e_65123:
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L42
	jp	__xcc_L44
__xcc_L42:
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
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, (_B)
	ld	e, -16(ix)
	ld	d, -15(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L43:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L41
__xcc_L44:
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L45:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #4
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74067
	ld	hl, #0
	jp	__cmp_e_3135
__cmp_t_74067:
	ld	hl, #1
__cmp_e_3135:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L46
	jp	__xcc_L48
__xcc_L46:
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
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, (_C)
	ld	e, -24(ix)
	ld	d, -23(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L47:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L45
__xcc_L48:
	ld	hl, #__str_49
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	hl, #4
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
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, #__str_50
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	.globl _PrintAll
	call	_PrintAll
	ld	hl, #__str_51
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
	ld	hl, (#_C + 6)
	push	hl
	ld	hl, (#_C + 4)
	push	hl
	ld	hl, (#_C + 2)
	push	hl
	ld	hl, (#_C)
	push	hl
	ld	hl, (#_B + 6)
	push	hl
	ld	hl, (#_B + 4)
	push	hl
	ld	hl, (#_B + 2)
	push	hl
	ld	hl, (#_B)
	push	hl
	ld	hl, (#_A + 6)
	push	hl
	ld	hl, (#_A + 4)
	push	hl
	ld	hl, (#_A + 2)
	push	hl
	ld	hl, (#_A)
	push	hl
	ld	hl, #4
	push	hl
	.globl _Hanoi
	call	_Hanoi
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
