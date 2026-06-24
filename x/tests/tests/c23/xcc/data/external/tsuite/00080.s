	.module xcc_output


	.area _CODE

	.globl _voidfn
_voidfn:
	; prologue: voidfn (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	jp	__voidfn_end
__voidfn_end:
	; epilogue: voidfn
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl _voidfn
	call	_voidfn
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
