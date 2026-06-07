	.module	xcc_output
	.area	_DATA
_main__data_0:
	.ds	64
_main__query_1:
	.ds	32
	.area	_CODE
_bench_seed_byte:
	; sdcccall(1) prologue: bench_seed_byte (locals=0, temp_frame=5, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	-1(ix), a
	ld	hl, #-5
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	ld	hl, (#65296)
	ld	-3(ix), l
	ld	-2(ix), h
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	l, -3(ix)
	ld	h, -2(ix)
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
	ld	-3(ix), l
	ld	-2(ix), h
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
	ld	l, -3(ix)
	ld	h, -2(ix)
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
	ld	a, -1(ix)
	add	a, #49
	ld	-2(ix), a
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	e, -2(ix)
	ld	d, #0
	add	hl, de
	ld	a, l
	ld	-1(ix), a
__bench_seed_byte_end:
	; epilogue: bench_seed_byte
	ld	sp, ix
	pop	ix
	ret
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=19, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-19
	add	hl, sp
	ld	sp, hl
	push	hl
	ld	a, #1
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-12(ix), a
	and	#7
	ld	-16(ix), a
	ld	hl, #_main__data_0
	ld	(hl), a
	ld	a, #1
	ld	-9(ix), a
__xcc_L3:
	ld	a, -9(ix)
	cp	#64
	jr	nc, __xcc_L7
__xcc_L4:
	ld	a, -9(ix)
	ld	l, a
	ld	h, #0
	dec	hl
	ld	-17(ix), l
	ld	-16(ix), h
	ld	hl, #_main__data_0
	ld	e, -17(ix)
	ld	d, -16(ix)
	add	hl, de
	ld	a, (hl)
	add	a, #1
	ld	-8(ix), a
	push	hl
	ld	a, -9(ix)
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-6(ix), a
	and	#3
	ld	-16(ix), a
	ld	a, -8(ix)
	ld	e, a
	ld	a, -16(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-17(ix), a
	ld	a, -9(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #_main__data_0
	ld	d, b
	ld	e, c
	add	hl, de
	push	hl
	ld	a, -17(ix)
	pop	hl
	ld	(hl), a
__xcc_L5:
	ld	a, -9(ix)
	add	a, #1
	ld	-9(ix), a
	jr	__xcc_L3
__xcc_L7:
	push	hl
	ld	a, #60
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-11(ix), a
	ld	-7(ix), a
	xor	a
	ld	-4(ix), a
__xcc_L10:
	ld	a, -4(ix)
	cp	#32
	jr	nc, __xcc_L9
__xcc_L11:
	ld	a, -7(ix)
	add	a, a
	add	a, a
	add	a, a
	ld	-16(ix), a
	ld	a, -7(ix)
	ld	e, a
	ld	a, -16(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-17(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-16(ix), a
	ld	a, -17(ix)
	ld	e, a
	ld	a, -16(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-18(ix), a
	ld	a, -4(ix)
	add	a, #102
	add	a, #17
	ld	-16(ix), a
	ld	a, -18(ix)
	ld	e, a
	ld	a, -16(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-7(ix), a
	ld	a, -4(ix)
	ld	l, a
	ld	h, #0
	ld	-17(ix), l
	ld	-16(ix), h
	ld	a, -7(ix)
	ld	e, a
	ld	a, -4(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-18(ix), a
	ld	hl, #_main__query_1
	ld	e, -17(ix)
	ld	d, -16(ix)
	add	hl, de
	ld	(hl), a
__xcc_L12:
	ld	a, -4(ix)
	add	a, #1
	ld	-4(ix), a
	jp	__xcc_L10
__xcc_L9:
	ld	hl, #22136
	ld	-3(ix), l
	ld	-2(ix), h
	xor	a
	ld	-9(ix), a
__xcc_L14:
	ld	a, -9(ix)
	cp	#32
	jp	nc, __xcc_L17
__xcc_L15:
	xor	a
	ld	-13(ix), a
	ld	a, #63
	ld	-1(ix), a
	xor	a
	ld	-10(ix), a
__xcc_L18:
	ld	a, -13(ix)
	ld	c, -1(ix)
	cp	c
	jr	z, __xcc_L19
	jr	c, __xcc_L19
	jp	__xcc_L20
__xcc_L19:
	ld	a, -13(ix)
	ld	e, a
	ld	a, -1(ix)
	ld	d, a
	ld	a, e
	add	a, d
	srl	a
	ld	-14(ix), a
	ld	e, a
	ld	d, #0
	ld	hl, #_main__data_0
	add	hl, de
	ld	a, (hl)
	ld	-5(ix), a
	ld	a, -9(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #_main__query_1
	add	hl, de
	ld	a, (hl)
	ld	-15(ix), a
	ld	a, -5(ix)
	ld	c, -15(ix)
	cp	c
	jr	nz, __xcc_L23
__xcc_L21:
	ld	a, #1
	ld	-10(ix), a
	ld	a, -14(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, l
	or	#0
	ld	l, a
	ld	a, h
	or	#128
	ld	h, a
	ld	-17(ix), l
	ld	-16(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -19(ix)
	ld	h, -18(ix)
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
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -19(ix)
	ld	h, -18(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-3(ix), l
	ld	-2(ix), h
	jr	__xcc_L20
__xcc_L23:
	ld	a, -5(ix)
	ld	c, -15(ix)
	cp	c
	jr	nc, __xcc_L25
__xcc_L24:
	ld	a, -14(ix)
	add	a, #1
	ld	-13(ix), a
	jp	__xcc_L18
__xcc_L25:
	ld	a, -14(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	h, b
	ld	l, c
	ld	a, h
	or	a, l
	jr	z, __xcc_L20
__xcc_L29:
	ld	a, -14(ix)
	sub	#1
	ld	-1(ix), a
__xcc_L26:
	jp	__xcc_L18
__xcc_L20:
	ld	a, -10(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	nz, __xcc_L16
__xcc_L30:
	ld	a, -13(ix)
	ld	l, a
	ld	h, #0
	ld	a, l
	or	#0
	ld	l, a
	ld	a, h
	or	#64
	ld	h, a
	ld	-17(ix), l
	ld	-16(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -19(ix)
	ld	h, -18(ix)
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
	ld	-19(ix), l
	ld	-18(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -19(ix)
	ld	h, -18(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-3(ix), l
	ld	-2(ix), h
__xcc_L32:
__xcc_L16:
	ld	a, -9(ix)
	add	a, #1
	ld	-9(ix), a
	jp	__xcc_L14
__xcc_L17:
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
