	.module	xcc_output
	.area	_DATA
_main__raw_0:
	.ds	120
_main__text_1:
	.ds	120
	.area	_CODE
_bench_mix16:
	; O3 sdcc-style leaf fast path: two-arg word mix helper
	ld	c, l
	ld	b, h
	ld	hl, #40503
	add	hl, de
	ld	a, c
	xor	a, l
	ld	c, a
	ld	a, b
	xor	a, h
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
	ex	de, hl
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=28, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-28
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ld	a, l
	xor	#142
	ld	l, a
	ld	a, h
	xor	#0
	ld	h, a
	ld	-26(ix), l
	ld	-25(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	-28(ix), l
	ld	-27(ix), h
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
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	e, #191
	ld	d, #0
	add	hl, de
	ld	a, l
	; O3 bench-fill loop (count=120)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__raw_0
__xcc_L6:
	ld	a, b
	cp	#120
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
	add	a, #229
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L6
__xcc_L5:
	xor	a
	ld	-6(ix), a
	ld	hl, #_main__text_1
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #_main__raw_0
	ld	-16(ix), l
	ld	-15(ix), h
__xcc_L10:
	ld	a, -6(ix)
	cp	#120
	jp	nc, __xcc_L13
__xcc_L11:
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, (hl)
	ld	-17(ix), a
	and	#7
	ld	b, a
	; O3 jump-table switch (7 cases, span=7)
	cp	#7
	jp	nc, __xcc_L21
	add	a, a
	ld	e, a
	ld	d, #0
	ld	hl, #__main_swtab_43
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	jp	(hl)
__main_swtab_43:
	.dw	__xcc_L14
	.dw	__xcc_L15
	.dw	__xcc_L16
	.dw	__xcc_L17
	.dw	__xcc_L18
	.dw	__xcc_L19
	.dw	__xcc_L20
__xcc_L14:
	ld	a, -17(ix)
	and	#15
	ld	-25(ix), a
	add	a, #97
	ld	-26(ix), a
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	(hl), a
	jr	__xcc_L12
__xcc_L15:
	ld	a, -17(ix)
	and	#15
	ld	-25(ix), a
	add	a, #65
	ld	-26(ix), a
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	(hl), a
	jr	__xcc_L12
__xcc_L16:
	ld	a, -17(ix)
	and	#7
	ld	-25(ix), a
	add	a, #48
	ld	-26(ix), a
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	(hl), a
	jr	__xcc_L12
__xcc_L17:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, #95
	ld	(hl), a
	jr	__xcc_L12
__xcc_L18:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, #45
	ld	(hl), a
	jr	__xcc_L12
__xcc_L19:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, #58
	ld	(hl), a
	jr	__xcc_L12
__xcc_L20:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, #32
	ld	(hl), a
	jr	__xcc_L12
__xcc_L21:
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, #44
	ld	(hl), a
__xcc_L22:
__xcc_L12:
	ld	a, -6(ix)
	add	a, #1
	ld	-6(ix), a
	ld	l, -24(ix)
	ld	h, -23(ix)
	inc	hl
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	inc	hl
	ld	-16(ix), l
	ld	-15(ix), h
	jp	__xcc_L10
__xcc_L13:
	ld	hl, #39612
	ld	-21(ix), l
	ld	-20(ix), h
	ld	hl, #0
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	ld	-19(ix), l
	ld	-18(ix), h
	xor	a
	ld	-22(ix), a
	xor	a
	ld	-6(ix), a
	ld	hl, #_main__text_1
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L23:
	ld	a, -6(ix)
	cp	#120
	jp	nc, __xcc_L26
__xcc_L24:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, (hl)
	ld	-5(ix), a
	xor	#128
	cp	#225
	jr	c, __xcc_L32
__xcc_L33:
	ld	a, -5(ix)
	xor	#128
	cp	#250
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L32:
	ld	a, -5(ix)
	xor	#128
	cp	#193
	jr	c, __xcc_L31
__xcc_L34:
	ld	a, -5(ix)
	xor	#128
	cp	#218
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L31:
	ld	a, -5(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L30
__xcc_L35:
	ld	a, -5(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L30:
	ld	a, -5(ix)
	cp	#95
	jr	z, __xcc_L27
	jr	__xcc_L28
__xcc_L27:
	ld	a, #1
	ld	-22(ix), a
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	a, -5(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -19(ix)
	ld	h, -18(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-19(ix), l
	ld	-18(ix), h
	jr	__xcc_L25
__xcc_L28:
	ld	a, -22(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	z, __xcc_L25
__xcc_L36:
	ld	e, -8(ix)
	ld	d, -7(ix)
	ld	l, -21(ix)
	ld	h, -20(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	e, -19(ix)
	ld	d, -18(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-21(ix), l
	ld	-20(ix), h
	ld	hl, #0
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	ld	-19(ix), l
	ld	-18(ix), h
	xor	a
	ld	-22(ix), a
__xcc_L38:
__xcc_L29:
__xcc_L25:
	ld	a, -6(ix)
	add	a, #1
	ld	-6(ix), a
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L23
__xcc_L26:
	ld	a, -22(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	z, __xcc_L41
__xcc_L39:
	ld	e, -8(ix)
	ld	d, -7(ix)
	ld	l, -21(ix)
	ld	h, -20(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-14(ix), l
	ld	-13(ix), h
	ld	e, -19(ix)
	ld	d, -18(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-21(ix), l
	ld	-20(ix), h
__xcc_L41:
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
