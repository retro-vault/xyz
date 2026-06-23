	.module xcc_output

	.area _DATA
	.globl _f
_f:
	.ds 2
	.globl _fprintfptr
_fprintfptr:
	.ds 2

	.area _CONST
__str_0:
	.db 121, 111, 32, 37, 100, 10, 0
__str_1:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _fred
_fred:
	; prologue: fred (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param p at 4(ix)
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #42
	jp	__fred_end
__fred_end:
	; epilogue: fred
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #24
	push	hl
	ld	hl, (_f)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, (_stdout)
	push	hl
	.globl __call_hl
	ld	hl, (_fprintfptr)
	call	__call_hl
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
