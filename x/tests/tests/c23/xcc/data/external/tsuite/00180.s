	.module xcc_output

	.area _CONST
__str_0:
	.db 97, 98, 99, 100, 101, 102, 0
__str_1:
	.db 37, 115, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=10)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
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
	.globl _strcpy
	call	_strcpy
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, -18(ix)
	ld	d, -17(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	push	ix
	pop	hl
	ld	de, #-22
	add	hl, de
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	.globl _printf
	call	_printf
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
