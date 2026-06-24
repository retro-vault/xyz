	.module xcc_output


	.area _CODE

	.globl _f
_f:
	; prologue: f (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param f at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	jp	__f_end
__f_end:
	; epilogue: f
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	push	hl
	.globl _f
	call	_f
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
