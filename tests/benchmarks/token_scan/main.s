	.module	xcc_output
	.area	_DATA
_main__raw_0:
	.ds	120
_main__text_1:
	.ds	120
	.area	_CODE
_bench_seed_word:
	; sdcccall(1) prologue: bench_seed_word (locals=0, temp_frame=0, stack_params=0)
	; frameless function: no IX frame needed
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ex	de, hl
__bench_seed_word_end:
	; epilogue: bench_seed_word
	ret
_bench_seed_byte:
	; sdcccall(1) prologue: bench_seed_byte (locals=0, temp_frame=7, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	-3(ix), a
	ld	hl, #-7
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	.globl	_bench_seed_word
	call	_bench_seed_word
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	a, -3(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-5(ix), l
	ld	-4(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-7(ix), l
	ld	-6(ix), h
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
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-5(ix), l
	ld	-4(ix), h
	ld	a, -3(ix)
	add	a, #49
	ld	-6(ix), a
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	e, -6(ix)
	ld	d, #0
	add	hl, de
	ld	a, l
	ld	-4(ix), a
__bench_seed_byte_end:
	; epilogue: bench_seed_byte
	ld	sp, ix
	pop	ix
	ret
_bench_mix16:
	; sdcccall(1) prologue: bench_mix16 (locals=0, temp_frame=4, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	-2(ix), l
	ld	-1(ix), h
	; keep incoming register arg t30 live in register for first use
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	ld	h, d
	ld	l, e
	; materialize incoming arg temp t30 for later reuse
	ld	-4(ix), e
	ld	-3(ix), d
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
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
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-4(ix), l
	ld	-3(ix), h
	ex	de, hl
__bench_mix16_end:
	; epilogue: bench_mix16
	ld	sp, ix
	pop	ix
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=21, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-21
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	push	hl
	ld	a, #142
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-8(ix), a
	ld	-16(ix), a
	xor	a
	ld	-10(ix), a
__xcc_L6:
	ld	a, -10(ix)
	cp	#120
	jr	nc, __xcc_L5
__xcc_L7:
	ld	a, -16(ix)
	add	a, a
	add	a, a
	add	a, a
	ld	-19(ix), a
	ld	a, -16(ix)
	ld	e, a
	ld	a, -19(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-20(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-19(ix), a
	ld	a, -20(ix)
	ld	e, a
	ld	a, -19(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-21(ix), a
	ld	a, -10(ix)
	add	a, #212
	add	a, #17
	ld	-19(ix), a
	ld	a, -21(ix)
	ld	e, a
	ld	a, -19(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-16(ix), a
	ld	a, -10(ix)
	ld	l, a
	ld	h, #0
	ld	-20(ix), l
	ld	-19(ix), h
	ld	a, -16(ix)
	ld	e, a
	ld	a, -10(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-21(ix), a
	ld	hl, #_main__raw_0
	ld	e, -20(ix)
	ld	d, -19(ix)
	add	hl, de
	ld	(hl), a
__xcc_L8:
	ld	a, -10(ix)
	add	a, #1
	ld	-10(ix), a
	jp	__xcc_L6
__xcc_L5:
	xor	a
	ld	-17(ix), a
__xcc_L10:
	ld	a, -17(ix)
	cp	#120
	jp	nc, __xcc_L13
__xcc_L11:
	ld	a, -17(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #_main__raw_0
	add	hl, de
	ld	a, (hl)
	ld	-18(ix), a
	and	#7
	ld	-13(ix), a
	; O3 jump-table switch (7 cases, span=7)
	cp	#7
	jp	nc, __xcc_L21
	add	a, a
	ld	e, a
	ld	d, #0
	ld	hl, #__main_swtab_34
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	jp	(hl)
__main_swtab_34:
	.dw	__xcc_L14
	.dw	__xcc_L15
	.dw	__xcc_L16
	.dw	__xcc_L17
	.dw	__xcc_L18
	.dw	__xcc_L19
	.dw	__xcc_L20
__xcc_L14:
	ld	a, -18(ix)
	and	#15
	ld	-19(ix), a
	add	a, #97
	ld	-20(ix), a
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jr	__xcc_L12
__xcc_L15:
	ld	a, -18(ix)
	and	#15
	ld	-19(ix), a
	add	a, #65
	ld	-20(ix), a
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jr	__xcc_L12
__xcc_L16:
	ld	a, -18(ix)
	and	#7
	ld	-19(ix), a
	add	a, #48
	ld	-20(ix), a
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jr	__xcc_L12
__xcc_L17:
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	a, #95
	ld	(hl), a
	jr	__xcc_L12
__xcc_L18:
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	a, #45
	ld	(hl), a
	jr	__xcc_L12
__xcc_L19:
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	a, #58
	ld	(hl), a
	jr	__xcc_L12
__xcc_L20:
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	a, #32
	ld	(hl), a
	jr	__xcc_L12
__xcc_L21:
	ld	hl, #_main__text_1
	ld	e, -17(ix)
	ld	d, #0
	add	hl, de
	ld	a, #44
	ld	(hl), a
__xcc_L22:
__xcc_L12:
	ld	a, -17(ix)
	add	a, #1
	ld	-17(ix), a
	jp	__xcc_L10
__xcc_L13:
	ld	hl, #39612
	ld	-7(ix), l
	ld	-6(ix), h
	ld	hl, #0
	ld	-5(ix), l
	ld	-4(ix), h
	ld	hl, #0
	ld	-12(ix), l
	ld	-11(ix), h
	xor	a
	ld	-3(ix), a
	xor	a
	ld	-17(ix), a
__xcc_L23:
	ld	a, -17(ix)
	cp	#120
	jp	nc, __xcc_L26
__xcc_L24:
	ld	a, -17(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #_main__text_1
	add	hl, de
	ld	a, (hl)
	ld	-9(ix), a
	xor	#128
	cp	#225
	jr	c, __xcc_L32
__xcc_L33:
	ld	a, -9(ix)
	xor	#128
	cp	#250
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L32:
	ld	a, -9(ix)
	xor	#128
	cp	#193
	jr	c, __xcc_L31
__xcc_L34:
	ld	a, -9(ix)
	xor	#128
	cp	#218
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L31:
	ld	a, -9(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L30
__xcc_L35:
	ld	a, -9(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L27
	jr	c, __xcc_L27
__xcc_L30:
	ld	a, -9(ix)
	cp	#95
	jr	z, __xcc_L27
	jr	__xcc_L28
__xcc_L27:
	ld	a, #1
	ld	-3(ix), a
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	ld	a, -9(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -12(ix)
	ld	h, -11(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-12(ix), l
	ld	-11(ix), h
	jr	__xcc_L25
__xcc_L28:
	ld	a, -3(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	z, __xcc_L25
__xcc_L36:
	ld	e, -5(ix)
	ld	d, -4(ix)
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-15(ix), l
	ld	-14(ix), h
	ld	e, -12(ix)
	ld	d, -11(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-7(ix), l
	ld	-6(ix), h
	ld	hl, #0
	ld	-5(ix), l
	ld	-4(ix), h
	ld	hl, #0
	ld	-12(ix), l
	ld	-11(ix), h
	xor	a
	ld	-3(ix), a
__xcc_L38:
__xcc_L29:
__xcc_L25:
	ld	a, -17(ix)
	add	a, #1
	ld	-17(ix), a
	jp	__xcc_L23
__xcc_L26:
	ld	a, -3(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	z, __xcc_L41
__xcc_L39:
	ld	e, -5(ix)
	ld	d, -4(ix)
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	e, -12(ix)
	ld	d, -11(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-7(ix), l
	ld	-6(ix), h
__xcc_L41:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
