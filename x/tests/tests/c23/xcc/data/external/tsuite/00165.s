	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 10, 0
__str_1:
	.db 37, 100, 44, 32, 37, 100, 44, 32, 37, 100, 10, 0


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
	ld	hl, #12
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
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	.globl __mul16
	ld	hl, #1
	push	hl
	ld	hl, #12
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #12
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	.globl __mul16
	ld	hl, #3
	push	hl
	ld	hl, #12
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
