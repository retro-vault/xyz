	.module	xcc_output
	.area	_CODE
_add_to:
	; sdcccall(1) prologue: add_to (locals=0, temp_frame=0, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	b, h
	ld	c, l
	push	de
	pop	hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	; receive (sdcccall1) register param already materialized in prologue
	; receive (sdcccall1) register param already materialized in prologue
	ld	h, b
	ld	l, c
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	h, b
	ld	l, c
	push	hl
	ld	e, -6(ix)
	ld	d, -5(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	h, b
	ld	l, c
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	ex	de, hl
__add_to_end:
	; epilogue: add_to
	ld	sp, ix
	pop	ix
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=2, temp_frame=0, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #4
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L0:
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	ld	b, h
	ld	c, l
	ld	de, #6
	ld	h, b
	ld	l, c
	.globl	_add_to
	call	_add_to
	push	de
	pop	hl
	ex	de, hl
	ld	hl, #10
	or	a, a
	sbc	hl, de
	jp	z, __xcc_L6
__xcc_L3:
	ld	hl, #1
	ex	de, hl
	jp	__main_end
__xcc_L6:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de, hl
	ld	hl, #10
	or	a, a
	sbc	hl, de
	jp	z, __xcc_L12
__xcc_L9:
	ld	hl, #2
	ex	de, hl
	jp	__main_end
__xcc_L12:
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	ld	b, h
	ld	c, l
	ld	de, #-3
	ld	h, b
	ld	l, c
	.globl	_add_to
	call	_add_to
	push	de
	pop	hl
	ex	de, hl
	ld	hl, #7
	or	a, a
	sbc	hl, de
	jp	z, __xcc_L14
__xcc_L15:
	ld	hl, #3
	ex	de, hl
	jp	__main_end
__xcc_L14:
	ld	hl, #0
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
