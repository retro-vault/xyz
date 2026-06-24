	.module xcc_output


	.area _CODE

	.globl _dr494
_dr494:
	; prologue: dr494 (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #12
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	ld	-6(ix), l
	ld	-5(ix), h
__dr494_end:
	; epilogue: dr494
	ld	sp, ix
	pop	ix
	ret
