	.module xcc_output


	.area _CODE

	.globl _f1
_f1:
	; prologue: f1 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param p at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__f1_end
__f1_end:
	; epilogue: f1
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=2003)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2003
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-1(ix), l
	ld	0(ix), h
	push	ix
	pop	hl
	ld	de, #-1
	add	hl, de
	dec	sp
	dec	sp
	ld	-2005(ix), l
	ld	-2004(ix), h
	ld	l, -2005(ix)
	ld	h, -2004(ix)
	push	hl
	.globl __call_hl
	ld	l, -2003(ix)
	ld	h, -2002(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-2007(ix), l
	ld	-2006(ix), h
	ld	l, -2007(ix)
	ld	h, -2006(ix)
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-2009(ix), l
	ld	-2008(ix), h
	ld	l, -2009(ix)
	ld	h, -2008(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
