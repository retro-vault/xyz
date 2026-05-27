	.module xcc_output

	.area _DATA
_main__q_0:
	.dw 0
	.db 0

	.area _CONST
__str_0:
	.db 110, 111, 110, 111, 110, 111, 0
__str_1:
	.db 98, 117, 103, 115, 0
__str_2:
	.db 110, 111, 110, 111, 110, 111, 0
__str_3:
	.db 98, 117, 103, 115, 0
__str_4:
	.db 110, 111, 110, 111, 110, 111, 0
__str_11:
	.db 98, 108, 97, 10, 0


	.area _CODE

	.globl _f1char
_f1char:
	; prologue: f1char (locals=12)
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
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-9(ix), l
	ld	-8(ix), h
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	e, -18(ix)
	ld	d, -17(ix)
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
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, -20(ix)
	ld	d, -19(ix)
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
	ld	l, -24(ix)
	ld	h, -23(ix)
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
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	jp	__f1char_end
__f1char_end:
	; epilogue: f1char
	ld	sp, ix
	pop	ix
	ret
	.globl _f1int
_f1int:
	; prologue: f1int (locals=13)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-13
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-15(ix), l
	ld	-14(ix), h
	ld	l, -15(ix)
	ld	h, -14(ix)
	ld	-9(ix), l
	ld	-8(ix), h
	push	ix
	pop	hl
	ld	de, #-13
	add	hl, de
	dec	sp
	dec	sp
	ld	-17(ix), l
	ld	-16(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	push	hl
	ld	e, -19(ix)
	ld	d, -18(ix)
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
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, -21(ix)
	ld	d, -20(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-25(ix), l
	ld	-24(ix), h
	ld	l, -25(ix)
	ld	h, -24(ix)
	push	hl
	ld	hl, #0
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
	ld	-27(ix), l
	ld	-26(ix), h
	ld	l, -27(ix)
	ld	h, -26(ix)
	jp	__f1int_end
__f1int_end:
	; epilogue: f1int
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=9)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-9
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-11(ix), l
	ld	-10(ix), h
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	-9(ix), l
	ld	-8(ix), h
	.globl _f1char
	call	_f1char
	dec	sp
	dec	sp
	ld	-13(ix), l
	ld	-12(ix), h
	ld	l, -13(ix)
	ld	h, -12(ix)
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
	ld	-15(ix), l
	ld	-14(ix), h
	ld	l, -15(ix)
	ld	h, -14(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L8
	jp	__xcc_L9
__xcc_L9:
	.globl _f1int
	call	_f1int
	dec	sp
	dec	sp
	ld	-17(ix), l
	ld	-16(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
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
	ld	-19(ix), l
	ld	-18(ix), h
	jp	__xcc_L10
__xcc_L8:
	ld	hl, #1
	ld	-19(ix), l
	ld	-18(ix), h
__xcc_L10:
	ld	l, -19(ix)
	ld	h, -18(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L5
	jp	__xcc_L7
__xcc_L5:
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-23(ix), l
	ld	-22(ix), h
	jp	__xcc_L7
__xcc_L7:
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
	ld	-25(ix), l
	ld	-24(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	e, -25(ix)
	ld	d, -24(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-27(ix), l
	ld	-26(ix), h
	ld	l, -27(ix)
	ld	h, -26(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-29(ix), l
	ld	-28(ix), h
	ld	l, -29(ix)
	ld	h, -28(ix)
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
	ld	-31(ix), l
	ld	-30(ix), h
	ld	l, -31(ix)
	ld	h, -30(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
