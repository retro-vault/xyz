	.module	xcc_output
	.optsdcc	-mz80 sdcccall(1)
	.area	_DATA
	.globl	__tmr_first
__tmr_first:
	.ds	2
	.area	_CODE
	.globl	_tmr_install
_tmr_install:
	; O3 sdcc-style helper fast path: timer install wrapper
	push	ix
	ld	ix, #0
	add	ix, sp
	push	af
	ld	c, l
	ld	b, h
	pop	hl
	push	de
	push	bc
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	de, #10
	ld	hl, #__tmr_first
	.globl	_so_create
	call	_so_create
	pop	bc
	ld	l, e
	ld	a, d
	or	a, l
	jr	z, __tmr_install_done_0
	ld	hl, #4
	add	hl, de
	ld	(hl), c
	inc	hl
	ld	(hl), b
	ld	hl, #8
	add	hl, de
	ld	c, l
	ld	b, h
	ld	hl, #6
	add	hl, de
	ld	a, -2(ix)
	ld	(hl), a
	inc	hl
	ld	a, -1(ix)
	ld	(hl), a
	ld	a, -2(ix)
	ld	(bc), a
	inc	bc
	ld	a, -1(ix)
	ld	(bc), a
__tmr_install_done_0:
	ld	sp, ix
	pop	ix
	pop	hl
	pop	af
	jp	(hl)
	.area	_CODE
	.globl	_tmr_uninstall
_tmr_uninstall:
	; naked: tmr_uninstall
	ex	de, hl
	ld	hl, #__tmr_first
	jp	_so_destroy
__tmr_uninstall_end:
	; naked epilogue: tmr_uninstall
	.area	_CODE
	.globl	__tmr_chain
__tmr_chain:
	; sdcccall(1) prologue: _tmr_chain (locals=0, temp_frame=10, stack_params=0)
	.globl	__sdcc_enter_ix
	call	__sdcc_enter_ix
	ld	hl, #-10
	add	hl, sp
	ld	sp, hl
	ld	hl, (#__tmr_first)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L3:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	z, __xcc_L5
__xcc_L4:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #8
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	a, d
	or	a, e
	jr	nz, __xcc_L7
__xcc_L6:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #6
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-6(ix), e
	ld	-5(ix), d
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #8
	add	hl, de
	ld	e, -6(ix)
	ld	d, -5(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #4
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	.globl	__sdcc_call_bc
	call	__sdcc_call_bc
	jr	__xcc_L8
__xcc_L7:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #8
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-8(ix), e
	ld	-7(ix), d
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	hl
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #8
	add	hl, de
	ld	e, -10(ix)
	ld	d, -9(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L8:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-2(ix), e
	ld	-1(ix), d
	jp	__xcc_L3
__xcc_L5:
___tmr_chain_end:
	; epilogue: _tmr_chain
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
