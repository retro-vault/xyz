	.module xcc_output

	.area _DATA
	.globl _g
_g:
	.ds 2


	.area _CODE

	.globl _f
_f:
	; prologue: f (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param i at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	(_g), hl
	ld	hl, #0
	jp	__f_end
__f_end:
	; epilogue: f
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	(_g), hl
	ld	hl, #2
	ld	(_g), hl
	ld	hl, #20
	ld	de, #40
	add	hl, de
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	push	hl
	.globl _f
	call	_f
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #2
	push	hl
	.globl _f
	call	_f
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #20
	ld	de, #40
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	(_g), hl
	ld	hl, #2
	ld	(_g), hl
	ld	hl, (_g)
	ld	hl, (_g)
	ex	de, hl
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
