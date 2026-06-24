	.module xcc_output


	.area _CODE

	.globl _foo
_foo:
	; prologue: foo (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	; receive param b at 6(ix)
	ld	hl, #2
	ld	e, 4(ix)
	ld	d, 5(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, 6(ix)
	ld	d, 7(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__foo_end
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #3
	push	hl
	ld	hl, #1
	push	hl
	.globl _foo
	call	_foo
	pop	bc
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
