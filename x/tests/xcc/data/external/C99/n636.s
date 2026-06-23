	.module xcc_output


	.area _CODE

	.globl _test
_test:
	; prologue: test (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl _frobble
	call	_frobble
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
__test_end:
	; epilogue: test
	ld	sp, ix
	pop	ix
	ret
