	.module xcc_output


	.area _CODE

	.globl _f2
_f2:
	; prologue: f2 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param c at 4(ix)
	; receive param b at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, 6(ix)
	ld	d, 7(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__f2_end
__f2_end:
	; epilogue: f2
	ld	sp, ix
	pop	ix
	ret
	.globl _f1
_f1:
	; prologue: f1 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
	; receive param b at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #_f2
	jp	__f1_end
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #0
	jp	__f1_end
__f1_end:
	; epilogue: f1
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
	ld	hl, #_f1
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #2
	push	hl
	ld	hl, #2
	push	hl
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	.globl __call_hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	call	__call_hl
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
