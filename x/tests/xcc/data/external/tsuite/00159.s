	.module xcc_output

	.area _CONST
__str_0:
	.db 97, 61, 37, 100, 10, 0
__str_1:
	.db 113, 102, 117, 110, 99, 40, 41, 10, 0
__str_2:
	.db 37, 100, 10, 0
__str_3:
	.db 37, 100, 10, 0


	.area _CODE

	.globl _myfunc
_myfunc:
	; prologue: myfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __mul16
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__myfunc_end
__myfunc_end:
	; epilogue: myfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _vfunc
_vfunc:
	; prologue: vfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param a at 4(ix)
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
__vfunc_end:
	; epilogue: vfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _qfunc
_qfunc:
	; prologue: qfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_1
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
__qfunc_end:
	; epilogue: qfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _zfunc
_zfunc:
	; prologue: zfunc (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __call_hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	__call_hl
__zfunc_end:
	; epilogue: zfunc
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_2
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #3
	push	hl
	.globl _myfunc
	call	_myfunc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #__str_3
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #4
	push	hl
	.globl _myfunc
	call	_myfunc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #1234
	push	hl
	.globl _vfunc
	call	_vfunc
	pop	bc
	.globl _qfunc
	call	_qfunc
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
