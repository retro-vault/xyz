	.module xcc_output

	.area _CONST
__str_0:
	.db 99, 104, 97, 114, 58, 32, 37, 99, 10, 0
__str_1:
	.db 105, 110, 116, 58, 32, 37, 100, 10, 0
__str_2:
	.db 102, 108, 111, 97, 116, 58, 32, 37, 102, 10, 0
__str_3:
	.db 37, 100, 32, 37, 100, 10, 0
__str_4:
	.db 37, 100, 32, 37, 100, 10, 0
__str_5:
	.db 37, 102, 32, 37, 102, 10, 0


	.area _CODE

	.globl _charfunc
_charfunc:
	; prologue: charfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	a, 4(ix)
	ld	l, a
	ld	h, #0
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
	ld	-4(ix), l
	ld	-3(ix), h
__charfunc_end:
	; epilogue: charfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _intfunc
_intfunc:
	; prologue: intfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
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
	ld	-4(ix), l
	ld	-3(ix), h
__intfunc_end:
	; epilogue: intfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _floatfunc
_floatfunc:
	; prologue: floatfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__floatfunc_end:
	; epilogue: floatfunc
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
	ld	hl, #97
	push	hl
	.globl _charfunc
	call	_charfunc
	pop	bc
	ld	hl, #98
	push	hl
	.globl _charfunc
	call	_charfunc
	pop	bc
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	.globl _charfunc
	call	_charfunc
	pop	bc
	pop	bc
	ld	hl, #97
	push	hl
	.globl _intfunc
	call	_intfunc
	pop	bc
	ld	hl, #98
	push	hl
	.globl _intfunc
	call	_intfunc
	pop	bc
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	.globl _intfunc
	call	_intfunc
	pop	bc
	pop	bc
	ld	hl, #97
	push	hl
	.globl _floatfunc
	call	_floatfunc
	pop	bc
	ld	hl, #98
	push	hl
	.globl _floatfunc
	call	_floatfunc
	pop	bc
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	.globl _floatfunc
	call	_floatfunc
	pop	bc
	pop	bc
	ld	hl, #97
	ld	-1(ix), l
	ld	0(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	0(ix), l
	ld	1(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	a, -2(ix)
	ld	l, a
	ld	h, #0
	push	hl
	ld	a, -1(ix)
	ld	l, a
	ld	h, #0
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #97
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, 2(ix)
	ld	h, 3(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, #97
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #97
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
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
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
