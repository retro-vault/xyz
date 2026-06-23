	.module xcc_output


	.area _CODE

	.globl _dr466
_dr466:
	; prologue: dr466 (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L0:
__xcc_L1:
	ld	hl, #1
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	ex	de, hl
	pop	hl
	jp	__dr466_end
__xcc_L2:
	jp	__xcc_L0
__xcc_L3:
__dr466_end:
	; epilogue: dr466
	ld	sp, ix
	pop	ix
	ret
