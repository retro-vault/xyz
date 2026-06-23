	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L5
__xcc_L5:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L8
__xcc_L8:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L11
__xcc_L9:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L11
__xcc_L11:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L14
__xcc_L12:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L14
__xcc_L14:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L17
__xcc_L15:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L17
__xcc_L17:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L18
	jp	__xcc_L20
__xcc_L18:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L20
__xcc_L20:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
