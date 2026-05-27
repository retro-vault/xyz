	.module xcc_output

	.area _DATA
	.globl _a
_a:
	.ds 2

	.area _CONST
__str_0:
	.db 37, 100, 10, 0
__str_1:
	.db 37, 100, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_3:
	.db 37, 100, 10, 0
__str_4:
	.db 37, 100, 10, 0
__str_5:
	.db 37, 100, 10, 0
__str_6:
	.db 37, 100, 10, 0
__str_7:
	.db 37, 100, 10, 0
__str_8:
	.db 37, 100, 10, 0
__str_9:
	.db 37, 100, 10, 0
__str_10:
	.db 37, 100, 10, 0
__str_11:
	.db 37, 115, 10, 0
__str_12:
	.db 108, 111, 110, 103, 0
__str_13:
	.db 37, 100, 10, 0
__str_14:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _a_f
_a_f:
	; prologue: a_f (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #20
	jp	__a_f_end
__a_f_end:
	; epilogue: a_f
	ld	sp, ix
	pop	ix
	ret
	.globl _b_f
_b_f:
	; prologue: b_f (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #10
	jp	__b_f_end
__b_f_end:
	; epilogue: b_f
	ld	sp, ix
	pop	ix
	ret
	.globl _foo
_foo:
	; prologue: foo (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param i at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	jp	__foo_end
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=14)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-14
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #2
	ld	-6(ix), l
	ld	-5(ix), h
	.globl _a_f
	call	_a_f
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	.globl _a_f
	call	_a_f
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #2
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	hl, #123
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #2
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-40(ix), l
	ld	-39(ix), h
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #5
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-52(ix), l
	ld	-51(ix), h
	ld	hl, #2
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #3
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-60(ix), l
	ld	-59(ix), h
	ld	hl, #4
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	hl, #1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_13
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
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
	ld	-74(ix), l
	ld	-73(ix), h
	ld	hl, #3
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
