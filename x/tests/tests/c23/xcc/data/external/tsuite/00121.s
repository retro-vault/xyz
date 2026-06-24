	.module xcc_output

	.area _DATA
	.globl _a
_a:
	.ds 2


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	push	hl
	.globl _f
	call	_f
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	push	hl
	.globl _g
	call	_g
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -4(ix)
	ld	d, -3(ix)
	or	a, a
	sbc	hl, de
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
	.globl _f
_f:
	; prologue: f (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	jp	__f_end
__f_end:
	; epilogue: f
	ld	sp, ix
	pop	ix
	ret
	.globl _g
_g:
	; prologue: g (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	jp	__g_end
__g_end:
	; epilogue: g
	ld	sp, ix
	pop	ix
	ret
