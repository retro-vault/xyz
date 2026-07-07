	.module	xcc_output
	.optsdcc	-mz80 sdcccall(1)
	.area	_CODE
_merge_with_next:
	; O3 sdcc-style helper fast path: merge with next block
	ld	c, l
	ld	b, h
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	bc
	push	de
	ld	l, c
	ld	h, b
	ld	bc, #5
	add	hl, bc
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	push	hl
	ld	l, e
	ld	h, d
	ld	bc, #5
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	pop	hl
	ld	de, #7
	add	hl, de
	add	hl, bc
	pop	de
	pop	bc
	push	bc
	push	de
	ld	l, c
	ld	h, b
	ld	bc, #5
	add	hl, bc
	pop	de
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	de
	ld	l, e
	ld	h, d
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	pop	hl
	pop	bc
	ld	h, b
	ld	l, c
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.area	_CODE
	.globl	_mem_init
_mem_init:
	; O3 sdcc-style helper fast path: fixed block init
	ld	c, l
	ld	b, h
	xor	a
	ld	(hl), a
	inc	hl
	ld	(hl), a
	ld	a, e
	add	a, #-7
	ld	e, a
	ld	a, d
	adc	a, #-1
	ld	d, a
	ld	h, b
	ld	l, c
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	h, b
	ld	l, c
	inc	hl
	inc	hl
	xor	a
	ld	(hl), a
	inc	hl
	ld	(hl), a
	ld	h, b
	ld	l, c
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	ld	(hl), a
	ret
	.area	_CODE
	.globl	_mem_allocate
_mem_allocate:
	; sdcccall(1) prologue: mem_allocate (locals=1, temp_frame=20, stack_params=2)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	; keep incoming register arg t80 live in register for first use
	ld	-5(ix), e
	ld	-4(ix), d
	ex	de, hl
	ld	hl, #-21
	add	hl, sp
	ld	sp, hl
	ex	de, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) param owner at 4(ix)
	xor	a
	ld	-1(ix), a
	ld	-7(ix), l
	ld	-6(ix), h
__xcc_L0:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	a, h
	or	a, l
	jp	z, __xcc_L2
__xcc_L1:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #4
	add	hl, de
	ld	a, (hl)
	and	#1
	ld	-8(ix), a
	or	a, a
	jp	nz, __xcc_L5
__xcc_L6:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #5
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-10(ix), e
	ld	-9(ix), d
	ld	l, -5(ix)
	ld	h, -4(ix)
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	c, __xcc_L5
__xcc_L3:
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, -5(ix)
	ld	d, -4(ix)
	or	a, a
	sbc	hl, de
	ld	b, h
	ld	c, l
	ld	de, #11
	or	a, a
	sbc	hl, de
	jp	z, __xcc_L9
	jp	c, __xcc_L9
__xcc_L7:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	de, #7
	add	hl, de
	ld	e, -5(ix)
	ld	d, -4(ix)
	add	hl, de
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	de, #5
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-16(ix), e
	ld	-15(ix), d
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	de, #7
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -16(ix)
	ld	h, -15(ix)
	or	a, a
	sbc	hl, bc
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	de, #5
	add	hl, de
	ld	e, -18(ix)
	ld	d, -17(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	inc	hl
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-20(ix), e
	ld	-19(ix), d
	ld	l, -14(ix)
	ld	h, -13(ix)
	inc	hl
	inc	hl
	ld	e, -20(ix)
	ld	d, -19(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	de, #4
	add	hl, de
	ld	a, (hl)
	ld	-21(ix), a
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	de, #4
	add	hl, de
	ld	(hl), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	de, #5
	add	hl, de
	ld	e, -5(ix)
	ld	d, -4(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, -14(ix)
	ld	d, -13(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L9:
	ld	l, -7(ix)
	ld	h, -6(ix)
	inc	hl
	inc	hl
	ld	e, 4(ix)
	ld	d, 5(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #4
	add	hl, de
	ld	a, #1
	ld	(hl), a
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #7
	add	hl, de
	ld	b, h
	ld	c, l
	ex	de, hl
	jr	__mem_allocate_end
__xcc_L5:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	ld	-7(ix), l
	ld	-6(ix), h
	ld	a, -1(ix)
	add	a, #1
	ld	-1(ix), a
	or	a, a
	jp	nz, __xcc_L0
__xcc_L2:
	ld	hl, #0
	ex	de, hl
__mem_allocate_end:
	; epilogue: mem_allocate
	ld	sp, ix
	pop	ix
	pop	bc
	inc	sp
	inc	sp
	push	bc
	ret
	.area	_CODE
	.globl	_mem_free
_mem_free:
	; sdcccall(1) prologue: mem_free (locals=1, temp_frame=14, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	-3(ix), l
	ld	-2(ix), h
	; keep incoming register arg t95 live in register for first use
	ld	hl, #-15
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	xor	a
	ld	-1(ix), a
	ex	de, hl
	ld	de, #7
	or	a, a
	sbc	hl, de
	ld	-7(ix), l
	ld	-6(ix), h
	ld	hl, #0
	ld	-9(ix), l
	ld	-8(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	-11(ix), l
	ld	-10(ix), h
__xcc_L13:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	a, h
	or	a, l
	jr	z, __xcc_L15
__xcc_L16:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ex	de, hl
	ld	l, -7(ix)
	ld	h, -6(ix)
	or	a, a
	sbc	hl, de
	jr	z, __xcc_L15
__xcc_L14:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	-9(ix), l
	ld	-8(ix), h
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	ld	-11(ix), l
	ld	-10(ix), h
	ld	a, -1(ix)
	add	a, #1
	ld	-1(ix), a
	or	a, a
	jr	nz, __xcc_L13
__xcc_L17:
	ld	hl, #0
	ex	de, hl
	jp	__mem_free_end
__xcc_L15:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ld	a, h
	or	a, l
	jr	nz, __xcc_L22
__xcc_L20:
	ld	hl, #0
	ex	de, hl
	jp	__mem_free_end
__xcc_L22:
	ld	l, -7(ix)
	ld	h, -6(ix)
	inc	hl
	inc	hl
	ld	de, #0
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #4
	add	hl, de
	xor	a
	ld	(hl), a
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	a, h
	or	a, l
	jr	z, __xcc_L25
__xcc_L26:
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	de, #4
	add	hl, de
	ld	a, (hl)
	and	#1
	ld	-12(ix), a
	or	a, a
	jr	nz, __xcc_L25
__xcc_L23:
	ld	l, -9(ix)
	ld	h, -8(ix)
	.globl	_merge_with_next
	call	_merge_with_next
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	-7(ix), l
	ld	-6(ix), h
__xcc_L25:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-14(ix), e
	ld	-13(ix), d
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	a, h
	or	a, l
	jr	z, __xcc_L29
__xcc_L30:
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	de, #4
	add	hl, de
	ld	a, (hl)
	and	#1
	ld	-15(ix), a
	or	a, a
	jr	nz, __xcc_L29
__xcc_L27:
	ld	l, -7(ix)
	ld	h, -6(ix)
	.globl	_merge_with_next
	call	_merge_with_next
__xcc_L29:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	de, #7
	add	hl, de
	ld	b, h
	ld	c, l
	ex	de, hl
__mem_free_end:
	; epilogue: mem_free
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
	.area	_CODE
	.globl	_mem_free_owner
_mem_free_owner:
	; sdcccall(1) prologue: mem_free_owner (locals=2, temp_frame=10, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	; keep incoming register arg t111 live in register for first use
	dec	sp
	dec	sp
	ld	-14(ix), e
	ld	-13(ix), d
	ex	de, hl
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ex	de, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	xor	a
	ld	-1(ix), a
	ld	-2(ix), a
	; materialize incoming arg temp t111 for later reuse
	ld	-4(ix), l
	ld	-3(ix), h
	ld	-6(ix), l
	ld	-5(ix), h
__xcc_L31:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	z, __xcc_L33
__xcc_L32:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #4
	add	hl, de
	ld	a, (hl)
	and	#1
	ld	-7(ix), a
	or	a, a
	jr	z, __xcc_L36
__xcc_L37:
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	inc	hl
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-9(ix), e
	ld	-8(ix), d
	ld	l, -14(ix)
	ld	h, -13(ix)
	or	a, a
	sbc	hl, de
	jr	nz, __xcc_L36
__xcc_L34:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #7
	add	hl, de
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -4(ix)
	ld	h, -3(ix)
	.globl	_mem_free
	call	_mem_free
	ld	h, d
	ld	l, e
	ld	-11(ix), l
	ld	-10(ix), h
	inc	-1(ix)
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	xor	a
	ld	-2(ix), a
	jr	__xcc_L31
__xcc_L36:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	ld	-6(ix), l
	ld	-5(ix), h
	ld	a, -2(ix)
	add	a, #1
	ld	-2(ix), a
	or	a, a
	jp	nz, __xcc_L31
__xcc_L33:
	ld	a, -1(ix)
	ld	-12(ix), a
__mem_free_owner_end:
	; epilogue: mem_free_owner
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
