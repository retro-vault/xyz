	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 32, 37, 100, 10, 0
__str_1:
	.db 37, 100, 32, 37, 100, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=3)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-3
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-5(ix), l
	ld	-4(ix), h
	ld	hl, #1
	push	hl
	ld	hl, #1
	push	hl
	ld	l, -5(ix)
	ld	h, -4(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7(ix), l
	ld	-6(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-9(ix), l
	ld	-8(ix), h
	ld	hl, #2
	push	hl
	ld	hl, #2
	push	hl
	ld	l, -9(ix)
	ld	h, -8(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-11(ix), l
	ld	-10(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
