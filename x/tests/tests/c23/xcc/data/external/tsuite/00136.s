	.module xcc_output

	.area _DATA
	.globl _a
_a:
	.ds 2
	.globl _b
_b:
	.ds 2
	.globl _c
_c:
	.ds 2
	.globl _d
_d:
	.ds 2
	.globl _e
_e:
	.ds 2
	.globl _f
_f:
	.ds 2
	.globl _e_
_e_:
	.ds 2
	.globl _f_
_f_:
	.ds 2
	.globl _e_
_e_:
	.ds 2
	.globl _f_
_f_:
	.ds 2


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
