	.module	xcc_output
	.area	_DATA
_main__code_0:
	.ds	96
_main__mem_1:
	.ds	16
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
	; sdcccall(1) prologue: main (locals=0, temp_frame=25, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-25
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	push	hl
	ld	a, #6
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-11(ix), a
	; O3 bench-fill loop (count=96)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__code_0
__xcc_L6:
	ld	a, b
	cp	#96
	jr	nc, __xcc_L10
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
	add	a, #109
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L6
__xcc_L10:
	push	hl
	ld	a, #55
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-6(ix), a
	; O3 bench-fill loop (count=16)
	ld	c, a
	ld	b, #0
	ld	hl, #_main__mem_1
__xcc_L13:
	ld	a, b
	cp	#16
	jr	nc, __xcc_L12
__xcc_L14:
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
	add	a, #126
	add	a, b
	ld	c, a
	xor	b
	ld	(hl), a
	inc	b
	inc	hl
	jr	__xcc_L13
__xcc_L12:
	xor	a
	ld	-18(ix), a
	push	hl
	ld	a, #1
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-8(ix), a
	ld	-9(ix), a
	push	hl
	ld	a, #2
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-16(ix), a
	ld	-17(ix), a
	push	hl
	ld	a, #3
	pop	hl
	.globl	_bench_seed_byte
	call	_bench_seed_byte
	ld	-19(ix), a
	ld	-20(ix), a
	ld	hl, #52719
	ld	-22(ix), l
	ld	-21(ix), h
__xcc_L17:
	ld	a, -18(ix)
	cp	#96
	jp	nc, __xcc_L19
__xcc_L18:
	ld	e, -18(ix)
	ld	d, #0
	ld	hl, #_main__code_0
	add	hl, de
	ld	a, (hl)
	ld	-4(ix), a
	and	#7
	ld	b, a
	; O3 jump-table switch (7 cases, span=7)
	cp	#7
	jp	nc, __xcc_L27
	add	a, a
	ld	e, a
	ld	d, #0
	ld	hl, #__main_swtab_71
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	jp	(hl)
__main_swtab_71:
	.dw	__xcc_L20
	.dw	__xcc_L21
	.dw	__xcc_L22
	.dw	__xcc_L23
	.dw	__xcc_L24
	.dw	__xcc_L25
	.dw	__xcc_L26
__xcc_L20:
	ld	a, -4(ix)
	ld	l, a
	ld	h, #0
	ld	a, l
	and	#15
	ld	l, a
	ld	a, h
	and	#0
	ld	h, a
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #_main__mem_1
	ld	e, -24(ix)
	ld	d, -23(ix)
	add	hl, de
	ld	a, (hl)
	ld	-23(ix), a
	ld	a, -9(ix)
	ld	e, a
	ld	a, -23(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-9(ix), a
	jp	__xcc_L28
__xcc_L21:
	ld	a, -4(ix)
	srl	a
	and	#15
	ld	-23(ix), a
	ld	e, -23(ix)
	ld	d, #0
	ld	hl, #_main__mem_1
	add	hl, de
	ld	a, (hl)
	ld	-24(ix), a
	ld	a, -9(ix)
	ld	e, a
	ld	a, -24(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-9(ix), a
	jp	__xcc_L28
__xcc_L22:
	ld	a, -17(ix)
	ld	e, a
	ld	a, -9(ix)
	ld	d, a
	ld	a, e
	add	a, d
	add	a, #1
	ld	-17(ix), a
	jp	__xcc_L28
__xcc_L23:
	ld	a, -9(ix)
	ld	e, a
	ld	a, -17(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-23(ix), a
	ld	a, -20(ix)
	ld	e, a
	ld	a, -23(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-20(ix), a
	jp	__xcc_L28
__xcc_L24:
	ld	a, -18(ix)
	and	#15
	ld	-23(ix), a
	ld	e, -23(ix)
	ld	d, #0
	ld	hl, #_main__mem_1
	add	hl, de
	ld	a, (hl)
	ld	e, a
	ld	a, -20(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-23(ix), a
	ld	a, -18(ix)
	and	#15
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	hl, #_main__mem_1
	add	hl, bc
	ld	a, -23(ix)
	ld	(hl), a
	jr	__xcc_L28
__xcc_L25:
	ld	a, -9(ix)
	and	#1
	ld	-23(ix), a
	or	a, a
	jr	z, __xcc_L28
__xcc_L32:
	ld	a, -18(ix)
	cp	#94
	jr	nc, __xcc_L28
__xcc_L29:
	ld	a, -18(ix)
	add	a, #1
	ld	-18(ix), a
__xcc_L31:
	jr	__xcc_L28
__xcc_L26:
	ld	a, -9(ix)
	add	a, a
	ld	-23(ix), a
	ld	a, -9(ix)
	ld	b, #7
__shiftb_9383:
	srl	a
	djnz	__shiftb_9383
	ld	-24(ix), a
	ld	a, -23(ix)
	ld	e, a
	ld	a, -24(ix)
	ld	d, a
	ld	a, e
	or	a, d
	ld	-9(ix), a
	jr	__xcc_L28
__xcc_L27:
	ld	a, -9(ix)
	ld	e, a
	ld	a, -17(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	e, a
	ld	a, -20(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-9(ix), a
__xcc_L28:
	ld	a, -17(ix)
	ld	h, a
	ld	a, -9(ix)
	ld	l, a
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -22(ix)
	ld	h, -21(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	a, -20(ix)
	ld	l, a
	ld	h, #0
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -2(ix)
	ld	h, -1(ix)
	.globl	_bench_mix16
	call	_bench_mix16
	push	de
	pop	hl
	ld	b, h
	ld	c, l
	ld	-22(ix), l
	ld	-21(ix), h
	ld	a, -18(ix)
	add	a, #1
	ld	-18(ix), a
	jp	__xcc_L17
__xcc_L19:
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	b, h
	ld	c, l
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
