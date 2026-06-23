	.module xcc_output

	.area _CONST
__str_3:
	.db 117, 110, 115, 105, 103, 110, 101, 100, 32, 101, 110, 117, 109, 32, 98, 105, 116, 45, 102, 105, 101, 108, 100, 115, 32, 98, 114, 111, 107, 101, 110, 10, 0


	.area _CODE

	.globl _blah
_blah:
	; prologue: blah (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	jp	__blah_end
__blah_end:
	; epilogue: blah
	ld	sp, ix
	pop	ix
	ret
	.globl _convert_like_real
_convert_like_real:
	; prologue: convert_like_real (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param convs at 4(ix)
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
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	hl, #152
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	.globl _blah
	call	_blah
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	jp	__convert_like_real_end
__xcc_L1:
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
__convert_like_real_end:
	; epilogue: convert_like_real
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	push	ix
	pop	hl
	ld	de, #0
	add	hl, de
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
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
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	de, #152
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #0
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _convert_like_real
	call	_convert_like_real
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
