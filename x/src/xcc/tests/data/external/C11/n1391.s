	.module xcc_output


	.area _CODE

	.globl _neg_zero
_neg_zero:
	; prologue: neg_zero (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, l
	dec	sp
	dec	sp
	ld	-4(ix), a
	ld	a, -4(ix)
	or	a, a
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
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
	jp	__xcc_L2
__xcc_L1:
	ld	hl, #1
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L2:
	ld	l, -8(ix)
	ld	h, -7(ix)
	jp	__neg_zero_end
__neg_zero_end:
	; epilogue: neg_zero
	ld	sp, ix
	pop	ix
	ret
	.globl _pos_inf
_pos_inf:
	; prologue: pos_inf (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl __fsdiv
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsdiv
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, l
	dec	sp
	dec	sp
	ld	-6(ix), a
	ld	a, -6(ix)
	or	a, a
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L5
__xcc_L4:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L5:
	ld	l, -8(ix)
	ld	h, -7(ix)
	jp	__pos_inf_end
__pos_inf_end:
	; epilogue: pos_inf
	ld	sp, ix
	pop	ix
	ret
	.globl _neg_inf
_neg_inf:
	; prologue: neg_inf (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; load_rr: unhandled operand_kind 4
	ld	hl, #0
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __fsdiv
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__fsdiv
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	push	de
	pop	hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, l
	dec	sp
	dec	sp
	ld	-8(ix), a
	ld	a, -8(ix)
	or	a, a
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L6:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	jp	__xcc_L8
__xcc_L7:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-10(ix), l
	ld	-9(ix), h
__xcc_L8:
	ld	l, -10(ix)
	ld	h, -9(ix)
	jp	__neg_inf_end
__neg_inf_end:
	; epilogue: neg_inf
	ld	sp, ix
	pop	ix
	ret
	.globl _nan
_nan:
	; prologue: nan (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl __fsdiv
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	call	__fsdiv
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, l
	dec	sp
	dec	sp
	ld	-6(ix), a
	ld	a, -6(ix)
	or	a, a
	jp	nz, __xcc_L9
	jp	__xcc_L10
__xcc_L9:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L11
__xcc_L10:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L11:
	ld	l, -8(ix)
	ld	h, -7(ix)
	jp	__nan_end
__nan_end:
	; epilogue: nan
	ld	sp, ix
	pop	ix
	ret
