	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L5
__xcc_L4:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L5:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L6:
	ld	hl, #0
	jp	__main_end
	jp	__xcc_L8
__xcc_L7:
	ld	hl, #1
	jp	__main_end
__xcc_L8:
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
