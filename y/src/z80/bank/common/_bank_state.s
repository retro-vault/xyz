	; Logical-bank state shared by every ZX backend.
	;
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih

	.module _bank_state
	.optsdcc -mz80 sdcccall(1)

	.globl  __bank_current
	.globl  __bank_count
	.globl  __bank_model
	.globl  __bank_map

	.area   _BSS
__bank_current::
	.ds     1
__bank_count::
	.ds     1
__bank_model::
	.ds     1

	; Startup patches this fixed-RAM JP to the detected machine's mapper.
__bank_map::
	.ds     3
