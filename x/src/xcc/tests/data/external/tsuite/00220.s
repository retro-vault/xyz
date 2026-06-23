	.module xcc_output

	.area _CONST
__str_0:
	.dw 104
	.dw 101
	.dw 108
	.dw 108
	.dw 111
	.dw 36
	.dw 36
	.dw 228
	.dw 189
	.dw 160
	.dw 229
	.dw 165
	.dw 189
	.dw 194
	.dw 162
	.dw 194
	.dw 162
	.dw 228
	.dw 184
	.dw 150
	.dw 231
	.dw 149
	.dw 140
	.dw 226
	.dw 130
	.dw 172
	.dw 226
	.dw 130
	.dw 172
	.dw 119
	.dw 111
	.dw 114
	.dw 108
	.dw 100
	.dw 0
__str_5:
	.db 37, 48, 52, 88, 32, 0
__str_6:
	.db 10, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	0(ix), l
	ld	1(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L1:
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L2
	jp	__xcc_L4
__xcc_L2:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L3:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L1
__xcc_L4:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
