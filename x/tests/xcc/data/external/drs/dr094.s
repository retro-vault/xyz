	.module xcc_output


	.area _CODE

	.globl _func
_func:
	; prologue: func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	ex	de, hl
	pop	hl
	jp	__func_end
__func_end:
	; epilogue: func
	ld	sp, ix
	pop	ix
	ret
	.globl _other_func
_other_func:
	; prologue: other_func (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	.globl _func
	call	_func
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	push	de
	pop	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	0(ix), l
	ld	1(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	0(ix), l
	ld	1(ix), h
__other_func_end:
	; epilogue: other_func
	ld	sp, ix
	pop	ix
	ret
