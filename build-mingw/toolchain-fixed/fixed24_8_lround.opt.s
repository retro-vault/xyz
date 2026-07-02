	; fixed24_8_lround.s
	; Round signed 24.8 to long/int result.
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih
	.module	fixed24_8_lround
	.optsdcc	-mz80 sdcccall(1)
	.globl	_fixed24_8_lround
	.globl	_fixed24_8_round
	.globl	_fixed24_8_to_int
	.area	_CODE
_fixed24_8_lround::
	call	_fixed24_8_round
	call	_fixed24_8_to_int
	ld	hl,#0
	bit	7,d
	ret	z
	dec	hl
	ret
