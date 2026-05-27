	.module xcc_output

	.area _CONST
__str_6:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_10:
	.db 101, 114, 114, 111, 114, 10, 0
__str_17:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_22:
	.db 101, 114, 114, 111, 114, 10, 0
__str_29:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_33:
	.db 101, 114, 114, 111, 114, 10, 0
__str_40:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_41:
	.db 101, 114, 114, 111, 114, 10, 0
__str_48:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_55:
	.db 116, 105, 109, 101, 111, 117, 116, 61, 37, 108, 100, 10, 0
__str_60:
	.db 116, 105, 109, 101, 111, 117, 116, 32, 105, 115, 32, 50, 0
__str_61:
	.db 116, 105, 109, 101, 111, 117, 116, 32, 105, 115, 32, 49, 0
__str_62:
	.db 116, 105, 109, 101, 111, 117, 116, 32, 105, 115, 32, 48, 63, 0
__str_63:
	.db 98, 101, 103, 105, 110, 10, 0
__str_64:
	.db 101, 110, 100, 10, 0


	.area _CODE

_kb_wait_1:
	; prologue: kb_wait_1 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L0:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L5
__xcc_L4:
__xcc_L7:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L8
	jp	__xcc_L9
__xcc_L8:
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	jp	__xcc_L7
__xcc_L9:
__xcc_L5:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L1:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L2:
__kb_wait_1_end:
	; epilogue: kb_wait_1
	ld	sp, ix
	pop	ix
	ret
_kb_wait_2:
	; prologue: kb_wait_2 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L11:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L14
	jp	__xcc_L15
__xcc_L14:
	ld	hl, #__str_17
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L16
__xcc_L15:
__xcc_L18:
__xcc_L19:
	ld	hl, #__str_22
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L20:
	jp	__xcc_L18
__xcc_L21:
__xcc_L16:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L12:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L11
	jp	__xcc_L13
__xcc_L13:
__kb_wait_2_end:
	; epilogue: kb_wait_2
	ld	sp, ix
	pop	ix
	ret
_kb_wait_2_1:
	; prologue: kb_wait_2_1 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L23:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L26
	jp	__xcc_L27
__xcc_L26:
	ld	hl, #__str_29
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L28
__xcc_L27:
__xcc_L30:
	ld	hl, #__str_33
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L31:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L30
	jp	__xcc_L32
__xcc_L32:
__xcc_L28:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L24:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L23
	jp	__xcc_L25
__xcc_L25:
__kb_wait_2_1_end:
	; epilogue: kb_wait_2_1
	ld	sp, ix
	pop	ix
	ret
_kb_wait_2_2:
	; prologue: kb_wait_2_2 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L34:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L37
	jp	__xcc_L38
__xcc_L37:
	ld	hl, #__str_40
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L39
__xcc_L38:
label:
	ld	hl, #__str_41
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	jp	label
__xcc_L39:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L35:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L34
	jp	__xcc_L36
__xcc_L36:
__kb_wait_2_2_end:
	; epilogue: kb_wait_2_2
	ld	sp, ix
	pop	ix
	ret
_kb_wait_3:
	; prologue: kb_wait_3 (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L42:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L45
	jp	__xcc_L46
__xcc_L45:
	ld	hl, #__str_48
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__xcc_L47
__xcc_L46:
	ld	hl, #1
	ld	-6(ix), l
	ld	-5(ix), h
	jp	label
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-6(ix), l
	ld	-5(ix), h
label:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #3
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	-6(ix), l
	ld	-5(ix), h
__xcc_L47:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L43:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L42
	jp	__xcc_L44
__xcc_L44:
__kb_wait_3_end:
	; epilogue: kb_wait_3
	ld	sp, ix
	pop	ix
	ret
_kb_wait_4:
	; prologue: kb_wait_4 (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #2
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L49:
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L52
	jp	__xcc_L53
__xcc_L52:
	ld	hl, #__str_55
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L54
__xcc_L53:
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #2
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
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L56
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L57
	jp	__xcc_L58
__xcc_L56:
	ld	hl, #__str_60
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	jp	__xcc_L59
__xcc_L57:
	ld	hl, #__str_61
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	jp	__xcc_L59
__xcc_L58:
	ld	hl, #__str_62
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
	jp	__xcc_L59
__xcc_L59:
__xcc_L54:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L50:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L49
	jp	__xcc_L51
__xcc_L51:
__kb_wait_4_end:
	; epilogue: kb_wait_4
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_63
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
	.globl _kb_wait_1
	call	_kb_wait_1
	.globl _kb_wait_2
	call	_kb_wait_2
	.globl _kb_wait_2_1
	call	_kb_wait_2_1
	.globl _kb_wait_2_2
	call	_kb_wait_2_2
	.globl _kb_wait_3
	call	_kb_wait_3
	.globl _kb_wait_4
	call	_kb_wait_4
	ld	hl, #__str_64
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
