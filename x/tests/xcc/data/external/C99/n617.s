	.module xcc_output


	.area _CODE

	.globl _test
_test:
	; prologue: test (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
__test_end:
	; epilogue: test
	ld	sp, ix
	pop	ix
	ret
