	.module xcc_output

	.area _CONST
__str_0:
	.db 73, 110, 32, 102, 114, 101, 100, 40, 41, 10, 0
__str_1:
	.db 73, 110, 32, 109, 105, 100, 100, 108, 101, 10, 0
__str_2:
	.db 65, 116, 32, 101, 110, 100, 10, 0
__str_3:
	.db 73, 110, 32, 106, 111, 101, 40, 41, 10, 0
__str_4:
	.db 99, 32, 61, 32, 37, 100, 10, 0
__str_5:
	.db 117, 104, 45, 111, 104, 10, 0
__str_6:
	.db 100, 111, 110, 101, 10, 0
__str_7:
	.db 73, 110, 32, 104, 101, 110, 114, 121, 40, 41, 10, 0
__str_8:
	.db 98, 32, 61, 32, 37, 100, 10, 0
__str_9:
	.db 100, 111, 110, 101, 10, 0


	.area _CODE

	.globl _fred
_fred:
	; prologue: fred (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	jp	done
	ld	hl, #__str_1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
done:
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
__fred_end:
	; epilogue: fred
	ld	sp, ix
	pop	ix
	ret
	.globl _joe
_joe:
	; prologue: joe (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #5678
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1234
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	jp	outer
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
outer:
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
__joe_end:
	; epilogue: joe
	ld	sp, ix
	pop	ix
	ret
	.globl _henry
_henry:
	; prologue: henry (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	inner
inner:
	ld	hl, #1234
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
__henry_end:
	; epilogue: henry
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	.globl _fred
	call	_fred
	.globl _joe
	call	_joe
	.globl _henry
	call	_henry
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
