	.module xcc_output


	.area _CODE

	.globl _dr209
_dr209:
	; prologue: dr209 (locals=30)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-30
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
__dr209_end:
	; epilogue: dr209
	ld	sp, ix
	pop	ix
	ret
