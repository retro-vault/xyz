	.module	xcc_output
	.optsdcc	-mz80 sdcccall(1)
	.area	_CODE
_copy_prefix:
	; sdcccall(1) prologue: copy_prefix (locals=1, temp_frame=5, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	-3(ix), l
	ld	-2(ix), h
	ld	-5(ix), e
	ld	-4(ix), d
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	xor	a
	ld	-1(ix), a
__xcc_L0:
	ld	a, -1(ix)
	cp	#3
	jr	nc, __xcc_L2
__xcc_L3:
	ld	c, -1(ix)
	ld	b, #0
	ld	l, -5(ix)
	ld	h, -4(ix)
	add	hl, bc
	ld	a, (hl)
	or	a, a
	jr	z, __xcc_L2
__xcc_L1:
	ld	c, -1(ix)
	ld	b, #0
	ld	l, -5(ix)
	ld	h, -4(ix)
	add	hl, bc
	ld	a, (hl)
	ld	-6(ix), a
	ld	c, -1(ix)
	ld	b, #0
	ld	l, -3(ix)
	ld	h, -2(ix)
	add	hl, bc
	ld	a, -6(ix)
	ld	(hl), a
	inc	-1(ix)
	jr	__xcc_L0
__xcc_L2:
	ld	c, -1(ix)
	ld	b, #0
	ld	l, -3(ix)
	ld	h, -2(ix)
	add	hl, bc
	xor	a
	ld	(hl), a
__copy_prefix_end:
	; epilogue: copy_prefix
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
	.area	_CODE
	.globl	_wrap_copy
_wrap_copy:
	; sdcccall(1) prologue: wrap_copy (locals=0, temp_frame=2, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	-2(ix), l
	ld	-1(ix), h
	; keep incoming register arg t1 live in BC for first use
	ld	b, d
	ld	c, e
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	; receive (sdcccall1) register param handled by prologue
	; receive (sdcccall1) register param handled by prologue
	ld	d, b
	ld	e, c
	ld	l, -2(ix)
	ld	h, -1(ix)
	.globl	_copy_prefix
	call	_copy_prefix
__wrap_copy_end:
	; epilogue: wrap_copy
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
