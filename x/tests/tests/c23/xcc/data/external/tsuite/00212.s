	.module xcc_output

	.area _CONST
__str_0:
	.db 75, 79, 32, 110, 111, 32, 95, 95, 42, 76, 80, 42, 95, 95, 32, 100, 101, 102, 105, 110, 101, 100, 46, 10, 0


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
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
