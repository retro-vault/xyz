	.module xcc_output

	.area _DATA
_f_global:
	.ds 8
_func__d_local_0:
	.ds 8


	.area _CODE

	.globl _func
_func:
	; prologue: func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, (#_f_global)
	ld	(#_func__d_local_0), hl
	ld	hl, (#_f_global + 2)
	ld	(#_func__d_local_0 + 2), hl
	ld	hl, (#_f_global + 4)
	ld	(#_func__d_local_0 + 4), hl
	ld	hl, (#_f_global + 6)
	ld	(#_func__d_local_0 + 6), hl
__func_end:
	; epilogue: func
	ld	sp, ix
	pop	ix
	ret
