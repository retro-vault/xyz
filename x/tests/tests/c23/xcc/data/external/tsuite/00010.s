	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
start:
	jp	next
	ld	hl, #1
	jp	__main_end
success:
	ld	hl, #0
	jp	__main_end
next:
foo:
	jp	success
	ld	hl, #1
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
