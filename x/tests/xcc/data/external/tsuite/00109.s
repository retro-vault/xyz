	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	jp	__xcc_L5
__xcc_L4:
	ld	hl, #0
	ld	-6(ix), l
	ld	-5(ix), h
__xcc_L5:
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L10
__xcc_L9:
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L11
__xcc_L10:
	ld	hl, #1
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L11:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #2
	jp	__main_end
	jp	__xcc_L8
__xcc_L8:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
