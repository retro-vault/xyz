	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
