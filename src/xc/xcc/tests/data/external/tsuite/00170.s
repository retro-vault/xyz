	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 32, 37, 100, 10, 0
__str_1:
	.db 37, 100, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_3:
	.db 101, 110, 117, 109, 32, 116, 111, 32, 105, 110, 116, 58, 32, 37, 117, 10, 0


	.area _CODE

	.globl _should_compile
_should_compile:
	; prologue: should_compile (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param s at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #_it_real_fn
	ex	de, hl
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__should_compile_end:
	; epilogue: should_compile
	ld	sp, ix
	pop	ix
	ret
	.globl _it_real_fn
_it_real_fn:
	; prologue: it_real_fn (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	jp	__it_real_fn_end
__it_real_fn_end:
	; epilogue: it_real_fn
	ld	sp, ix
	pop	ix
	ret
_deref_uintptr:
	; prologue: deref_uintptr (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param p at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__deref_uintptr_end
__deref_uintptr_end:
	; epilogue: deref_uintptr
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #75
	push	hl
	ld	hl, #74
	push	hl
	ld	hl, #73
	push	hl
	ld	hl, #54
	push	hl
	ld	hl, #3
	push	hl
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	ld	hl, #0
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #12
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #54
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _deref_uintptr
	call	_deref_uintptr
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
