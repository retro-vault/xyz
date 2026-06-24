	.module xcc_output

	.area _CONST
__str_0:
	.db 97, 98, 111, 114, 116, 32, 61, 32, 37, 115, 10, 0
__str_1:
	.db 49, 49, 49, 0
__str_2:
	.db 97, 98, 111, 114, 116, 32, 61, 32, 37, 115, 10, 0
__str_3:
	.db 50, 50, 50, 0
__str_4:
	.db 97, 98, 111, 114, 116, 32, 61, 32, 37, 115, 10, 0
__str_5:
	.db 51, 51, 51, 0
__str_6:
	.db 97, 98, 111, 114, 116, 32, 61, 32, 37, 115, 10, 0
__str_7:
	.db 51, 51, 51, 0
__str_8:
	.db 97, 98, 111, 114, 116, 32, 61, 32, 37, 115, 10, 0
__str_9:
	.db 51, 51, 51, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #__str_5
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
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #__str_9
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
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
