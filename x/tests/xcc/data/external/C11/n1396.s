	.module xcc_output


	.area _CODE

	.globl _extended_float_func
_extended_float_func:
	; prologue: extended_float_func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de, hl
	pop	hl
	jp	__extended_float_func_end
__extended_float_func_end:
	; epilogue: extended_float_func
	ld	sp, ix
	pop	ix
	ret
	.globl _extended_float_func_cast
_extended_float_func_cast:
	; prologue: extended_float_func_cast (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	pop	hl
	jp	__extended_float_func_cast_end
__extended_float_func_cast_end:
	; epilogue: extended_float_func_cast
	ld	sp, ix
	pop	ix
	ret
	.globl _extended_double_func
_extended_double_func:
	; prologue: extended_double_func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de, hl
	pop	hl
	jp	__extended_double_func_end
__extended_double_func_end:
	; epilogue: extended_double_func
	ld	sp, ix
	pop	ix
	ret
	.globl _extended_double_func_cast
_extended_double_func_cast:
	; prologue: extended_double_func_cast (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	pop	hl
	jp	__extended_double_func_cast_end
__extended_double_func_cast_end:
	; epilogue: extended_double_func_cast
	ld	sp, ix
	pop	ix
	ret
	.globl _float_source_func
_float_source_func:
	; prologue: float_source_func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de, hl
	pop	hl
	jp	__float_source_func_end
__float_source_func_end:
	; epilogue: float_source_func
	ld	sp, ix
	pop	ix
	ret
	.globl _float_source_func_cast
_float_source_func_cast:
	; prologue: float_source_func_cast (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	pop	hl
	jp	__float_source_func_cast_end
__float_source_func_cast_end:
	; epilogue: float_source_func_cast
	ld	sp, ix
	pop	ix
	ret
	.globl _double_source_func
_double_source_func:
	; prologue: double_source_func (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de, hl
	pop	hl
	jp	__double_source_func_end
__double_source_func_end:
	; epilogue: double_source_func
	ld	sp, ix
	pop	ix
	ret
	.globl _double_source_func_cast
_double_source_func_cast:
	; prologue: double_source_func_cast (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	.globl __fsmul
	ld	l, 2(ix)
	ld	h, 3(ix)
	push	hl
	ld	l, 0(ix)
	ld	h, 1(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__fsmul
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	push	de
	pop	hl
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	ex	de, hl
	pop	hl
	jp	__double_source_func_cast_end
__double_source_func_cast_end:
	; epilogue: double_source_func_cast
	ld	sp, ix
	pop	ix
	ret
