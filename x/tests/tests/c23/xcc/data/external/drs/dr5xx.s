	.module xcc_output


	.area _CODE

	.globl _dr502
_dr502:
	; prologue: dr502 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
__dr502_end:
	; epilogue: dr502
	ld	sp, ix
	pop	ix
	ret
