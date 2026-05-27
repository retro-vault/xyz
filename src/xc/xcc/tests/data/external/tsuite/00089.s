	.module xcc_output

	.area _DATA
	.globl _s
_s:
	.dw 0


	.area _CODE

	.globl _zero
_zero:
	; prologue: zero (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	jp	__zero_end
__zero_end:
	; epilogue: zero
	ld	sp, ix
	pop	ix
	ret
	.globl _anon
_anon:
	; prologue: anon (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #_s
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__anon_end
__anon_end:
	; epilogue: anon
	ld	sp, ix
	pop	ix
	ret
	.globl _go
_go:
	; prologue: go (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #_anon
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__go_end
__go_end:
	; epilogue: go
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl _go
	call	_go
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __call_hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	__call_hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	.globl __call_hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	call	__call_hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
