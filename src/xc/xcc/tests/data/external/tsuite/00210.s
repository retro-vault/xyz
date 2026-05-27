	.module xcc_output

	.area _CONST
__str_0:
	.db 37, 105, 10, 0
__str_1:
	.db 37, 105, 10, 0


	.area _CODE

	.globl _foo
_foo:
	; prologue: foo (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _actual_function
_actual_function:
	; prologue: actual_function (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #42
	jp	__actual_function_end
__actual_function_end:
	; epilogue: actual_function
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=6)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-6
	add	hl, sp
	ld	sp, hl
	ld	hl, #_actual_function
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	.globl __call_hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	call	__call_hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	.globl __call_hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	call	__call_hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
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
