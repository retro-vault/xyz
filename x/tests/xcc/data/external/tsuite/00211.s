	.module xcc_output

	.area _DATA
	.globl _array
_array:
	.ds 2

	.area _CONST
__str_0:
	.db 110, 43, 49, 32, 61, 32, 37, 100, 10, 0


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
	ld	hl, #14
	inc	hl
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
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
