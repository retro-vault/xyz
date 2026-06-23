	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 10, 0
__str_1:
	.db 37, 100, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_3:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=7)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-7
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-9(ix), l
	ld	-8(ix), h
	ld	hl, #1
	push	hl
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-11(ix), l
	ld	-10(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-13(ix), l
	ld	-12(ix), h
	ld	hl, #2
	push	hl
	ld	l, -13(ix)
	ld	h, -12(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-15(ix), l
	ld	-14(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-17(ix), l
	ld	-16(ix), h
	ld	hl, #4
	push	hl
	ld	l, -17(ix)
	ld	h, -16(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-19(ix), l
	ld	-18(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-21(ix), l
	ld	-20(ix), h
	ld	hl, #0
	push	hl
	ld	l, -21(ix)
	ld	h, -20(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-23(ix), l
	ld	-22(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
