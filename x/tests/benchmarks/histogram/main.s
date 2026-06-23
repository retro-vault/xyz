	.module	xcc_output
	.area	_DATA
_main__input_0:
	.ds	128
_main__bins_1:
	.ds	16
	.area	_CODE
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=14, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-14
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ld	a, l
	xor	#195
	ld	l, a
	ld	a, h
	xor	#0
	ld	h, a
	ld	-11(ix), l
	ld	-10(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	-13(ix), l
	ld	-12(ix), h
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	ld	b, h
	ld	c, l
	ld	l, -13(ix)
	ld	h, -12(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	e, #244
	ld	d, #0
	add	hl, de
	ld	a, l
	; O3 bench-fill loop (count=128)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__input_0
__xcc_L6:
	ld	a, b
	cp	#128
	jr	nc, __xcc_L5
__xcc_L7:
	ld	a, c
	add	a, a
	add	a, a
	add	a, a
	xor	c
	ld	d, a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	xor	d
	add	a, #170
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L6
__xcc_L5:
	; O3 byte zero loop (count=16)
	ld	hl, #_main__bins_1
	ld	b, #16
	xor	a
__xcc_L10:
__xcc_L11:
	ld	(hl), a
	inc	hl
	djnz	__xcc_L11
__xcc_L13:
	xor	a
	ld	-9(ix), a
__xcc_L14:
	ld	a, -9(ix)
	cp	#128
	jr	nc, __xcc_L17
__xcc_L15:
	ld	e, -9(ix)
	ld	d, #0
	ld	hl, #_main__input_0
	add	hl, de
	ld	a, (hl)
	ld	-10(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-11(ix), a
	ld	e, -11(ix)
	ld	d, #0
	ld	hl, #_main__bins_1
	add	hl, de
	ld	a, (hl)
	add	a, #1
	ld	-11(ix), a
	ld	a, -10(ix)
	srl	a
	srl	a
	srl	a
	srl	a
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #_main__bins_1
	add	hl, bc
	ld	a, -11(ix)
	ld	(hl), a
__xcc_L16:
	ld	a, -9(ix)
	add	a, #1
	ld	-9(ix), a
	jr	__xcc_L14
__xcc_L17:
	; O3 bench-mix-array loop (count=16)
	ld	hl, #26505
	ld	c, #0
__xcc_L18:
	ld	a, c
	cp	#16
	jr	nc, __xcc_L21
__xcc_L19:
	ld	a, #<(_main__bins_1)
	add	a, c
	ld	e, a
	ld	a, #>(_main__bins_1)
	adc	a, #0
	ld	d, a
	ld	a, (de)
	ld	d, c
	ld	e, a
	ld	c, l
	ld	b, h
	ld	hl, #40503
	add	hl, de
	ld	a, c
	xor	l
	ld	c, a
	ld	a, b
	xor	h
	ld	b, a
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	a, e
	xor	#74
	ld	c, a
	ld	a, d
	xor	#127
	ld	b, a
	add	hl, bc
	ld	a, d
	inc	a
	ld	c, a
	jr	__xcc_L18
__xcc_L21:
	ld	-2(ix), l
	ld	-1(ix), h
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
