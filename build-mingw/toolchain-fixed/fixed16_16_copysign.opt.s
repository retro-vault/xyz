	; fixed16_16_copysign.s
	; Copy sign for signed 16.16 fixed values.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed16_16_copysign
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed16_16_copysign
	.globl	_fixed16_16_abs
	.globl	_fixed16_16_neg
	.area	_CODE
	; inputs:  DE:HL = magnitude, stack = sign source
	; outputs: DE:HL = copysign(magnitude, sign)
_fixed16_16_copysign::
	push	ix
	ld	ix,#0
	add	ix,sp
	call	_fixed16_16_abs
	bit	7,7(ix)
	jr	z,.done
	call	_fixed16_16_neg
.done:
	pop	ix
	ret
