	.module xcc_output

	.area _DATA
	.globl _fc
_fc:
	.ds 8
	.globl _dc
_dc:
	.ds 8
	.globl _ldc
_ldc:
	.ds 8


	.area _CODE

	.globl _func
_func:
	; prologue: func (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive param n at 4(ix)
	; receive param m at 6(ix)
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	ld	hl, #0
	add	hl, sp
	or	a, a
	sbc	hl, de
	ld	sp, hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
__func_end:
	; epilogue: func
	ld	sp, ix
	pop	ix
	ret
