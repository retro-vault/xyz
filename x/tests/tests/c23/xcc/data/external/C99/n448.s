	.module xcc_output

	.area _DATA
	.globl _ipr
_ipr:
	.ds 2
	.globl _ir
_ir:
	.ds 2
	.globl _fp
_fp:
	.ds 2
	.globl _ipr2
_ipr2:
	.ds 2


	.area _CODE

	.globl _f
_f:
	; prologue: f (locals=8)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	; receive param array at 4(ix)
	ld	hl, (_ipr)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-8(ix), l
	ld	-7(ix), h
__f_end:
	; epilogue: f
	ld	sp, ix
	pop	ix
	ret
