	; Spectrum 48K logical-bank backend (one no-op bank).
	;
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih

	.module _bank_map_48
	.optsdcc -mz80 sdcccall(1)

	.globl  __bank_map_48
	.globl  __bank_current

	.area   _CODE

	; input: A = logical bank (only zero is valid)
	; output: selected logical bank recorded in common RAM.
__bank_map_48::
	ld      (__bank_current),a
	ret
