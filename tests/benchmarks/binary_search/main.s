	.module	xcc_output
	.area	_DATA
_main__data_0:
	.ds	64
_main__query_1:
	.ds	32
	.area	_CODE
_bench_seed_byte:
	; O3 sdcc-style leaf fast path: byte seed helper
	ld	c, a
	ld	hl, (#65296)
	ld	a, l
	xor	a, c
	ld	l, a
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, l
	xor	a, e
	ld	e, a
	ld	a, h
	xor	a, d
	ld	d, a
	ld	a, e
	ld	l, d
	ld	b, #5
__bench_seed_byte_seed_shift:
	srl	l
	rr	a
	djnz	__bench_seed_byte_seed_shift
	xor	a, e
	ld	e, a
	ld	a, l
	xor	a, d
	ld	d, a
	ld	b, #0
	ld	hl, #49
	add	hl, bc
	add	hl, de
	ld	a, l
	ret
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
	; sdcccall(1) prologue: main (locals=0, temp_frame=20, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-20
	add	hl, sp
	ld	sp, hl
	; O3 seeded recurrence loop (count=64)
	ld	a, #1
	call	_bench_seed_byte
	and	#7
	ld	(_main__data_0), a
	ld	hl, #_main__data_0 + 1
	ld	c, #1
__xcc_L3:
__xcc_L4:
	push	hl
	push	bc
	ld	a, c
	call	_bench_seed_byte
	pop	bc
	pop	hl
	and	#3
	ld	b, a
	dec	hl
	ld	a, (hl)
	inc	hl
	inc	a
	add	a, b
	ld	(hl), a
	inc	hl
	inc	c
	ld	a, c
	cp	#64
	jr	c, __xcc_L4
__xcc_L7:
	push	hl
	ld	a, #60
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-12(ix), a
	; O3 bench-fill loop (count=32)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__query_1
__xcc_L10:
	ld	a, b
	cp	#32
	jr	nc, __xcc_L9
__xcc_L11:
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
	add	a, #119
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L10
__xcc_L9:
	ld	hl, #22136
	ld	-7(ix), l
	ld	-6(ix), h
	xor	a
	ld	-13(ix), a
__xcc_L14:
	ld	a, -13(ix)
	cp	#32
	jp	nc, __xcc_L17
__xcc_L15:
	xor	a
	ld	-5(ix), a
	ld	a, #63
	ld	-4(ix), a
	xor	a
	ld	-11(ix), a
__xcc_L18:
	ld	a, -5(ix)
	ld	c, -4(ix)
	cp	c
	jr	z, __xcc_L19
	jr	c, __xcc_L19
	jp	__xcc_L20
__xcc_L19:
	ld	a, -5(ix)
	ld	e, a
	ld	a, -4(ix)
	ld	d, a
	ld	a, e
	add	a, d
	srl	a
	ld	-3(ix), a
	ld	e, -3(ix)
	ld	d, #0
	ld	hl, #_main__data_0
	add	hl, de
	ld	a, (hl)
	ld	-2(ix), a
	ld	e, -13(ix)
	ld	d, #0
	ld	hl, #_main__query_1
	add	hl, de
	ld	a, (hl)
	ld	-1(ix), a
	ld	a, -2(ix)
	ld	c, -1(ix)
	cp	c
	jr	nz, __xcc_L23
__xcc_L21:
	ld	a, #1
	ld	-11(ix), a
	ld	a, -3(ix)
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
	ld	-19(ix), l
	ld	-18(ix), h
	ld	e, -19(ix)
	ld	d, -18(ix)
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-7(ix), l
	ld	-6(ix), h
	jr	__xcc_L20
__xcc_L23:
	ld	a, -2(ix)
	ld	c, -1(ix)
	cp	c
	jr	nc, __xcc_L25
__xcc_L24:
	ld	a, -3(ix)
	add	a, #1
	ld	-5(ix), a
	jp	__xcc_L18
__xcc_L25:
	ld	a, -3(ix)
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
	ld	a, -3(ix)
	sub	#1
	ld	-4(ix), a
__xcc_L26:
	jp	__xcc_L18
__xcc_L20:
	ld	a, -11(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	jr	nz, __xcc_L16
__xcc_L30:
	ld	a, -5(ix)
	ld	l, a
	ld	h, #0
	ld	a, l
	or	#0
	ld	l, a
	ld	a, h
	or	#64
	ld	h, a
	ld	-19(ix), l
	ld	-18(ix), h
	ld	e, -19(ix)
	ld	d, -18(ix)
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-7(ix), l
	ld	-6(ix), h
__xcc_L32:
__xcc_L16:
	ld	a, -13(ix)
	add	a, #1
	ld	-13(ix), a
	jp	__xcc_L14
__xcc_L17:
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
