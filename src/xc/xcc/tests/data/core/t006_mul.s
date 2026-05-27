	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #6
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #7
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __mul16
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
