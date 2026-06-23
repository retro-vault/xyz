	.module xcc_output


	.area _CODE

	.globl _test_decimal_constants
_test_decimal_constants:
	; prologue: test_decimal_constants (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
__test_decimal_constants_end:
	; epilogue: test_decimal_constants
	ld	sp, ix
	pop	ix
	ret
	.globl _test_octal_constants
_test_octal_constants:
	; prologue: test_octal_constants (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
__test_octal_constants_end:
	; epilogue: test_octal_constants
	ld	sp, ix
	pop	ix
	ret
	.globl _test_hexadecimal_constants
_test_hexadecimal_constants:
	; prologue: test_hexadecimal_constants (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
__test_hexadecimal_constants_end:
	; epilogue: test_hexadecimal_constants
	ld	sp, ix
	pop	ix
	ret
