	.module xcc_output

	.area _DATA
	.globl _t
_t:
	.ds 2

	.area _CONST
__str_0:
	.db 100, 97, 116, 97, 32, 61, 32, 34, 37, 115, 34, 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=20)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-20
	add	hl, sp
	ld	sp, hl
	ld	hl, (_t)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #4
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #5
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #12
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -10(ix)
	ld	d, -9(ix)
	add	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, -20(ix)
	ld	d, -19(ix)
	or	a, a
	sbc	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	e, -40(ix)
	ld	d, -39(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, -10(ix)
	ld	d, -9(ix)
	add	hl, de
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	ld	e, -42(ix)
	ld	d, -41(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
