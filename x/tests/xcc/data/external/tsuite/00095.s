	.module xcc_output

	.area _DATA
	.globl _x
_x:
	.ds 2
	.globl _x
_x:
	.dw 3
	.globl _x
_x:
	.ds 2


	.area _CODE

	.globl _foo
_foo:
	; prologue: foo (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #_main
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__foo_end
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, (_x)
	push	hl
	ld	hl, #3
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #0
	jp	__main_end
	jp	__xcc_L2
__xcc_L2:
	ld	hl, #0
	ld	(_x), hl
	ld	hl, (_x)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
