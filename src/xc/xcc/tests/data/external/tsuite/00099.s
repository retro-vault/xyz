	.module xcc_output


	.area _CODE

_vecresize:
	; prologue: vecresize (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param v at 4(ix)
	; receive param cap at 6(ix)
	jp	__vecresize_end
__vecresize_end:
	; epilogue: vecresize
	ld	sp, ix
	pop	ix
	ret
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
