	.module xcc_output


	.area _CODE

	.globl _func
_func:
	; prologue: func (locals=1)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-1
	add	hl, sp
	ld	sp, hl
	.globl _f
	call	_f
	ld	-101(ix), l
	ld	-100(ix), h
	push	ix
	pop	hl
	ld	de, #-101
	add	hl, de
	dec	sp
	dec	sp
	ld	-103(ix), l
	ld	-102(ix), h
	ld	l, -103(ix)
	ld	h, -102(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-105(ix), l
	ld	-104(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #10
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-107(ix), l
	ld	-106(ix), h
	ld	l, -105(ix)
	ld	h, -104(ix)
	ld	e, -107(ix)
	ld	d, -106(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-109(ix), l
	ld	-108(ix), h
	ld	l, -109(ix)
	ld	h, -108(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-111(ix), l
	ld	-110(ix), h
	ld	l, -111(ix)
	ld	h, -110(ix)
	ld	-1(ix), l
	ld	0(ix), h
__func_end:
	; epilogue: func
	ld	sp, ix
	pop	ix
	ret
