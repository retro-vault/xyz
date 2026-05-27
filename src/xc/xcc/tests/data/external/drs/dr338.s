	.module xcc_output


	.area _CODE

	.globl _dr338
_dr338:
	; prologue: dr338 (locals=1)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-1
	add	hl, sp
	ld	sp, hl
	ld	l, -1(ix)
	ld	h, 0(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-3(ix), l
	ld	-2(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-5(ix), l
	ld	-4(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	jp	__dr338_end
__dr338_end:
	; epilogue: dr338
	ld	sp, ix
	pop	ix
	ret
