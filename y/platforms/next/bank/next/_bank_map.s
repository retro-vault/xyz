	; Spectrum Next logical-bank backend (two 8 KiB MMU pages).
	;
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih

	.module _bank_map_next
	.optsdcc -mz80 sdcccall(1)

	.globl  __bank_map_next
	.globl  __bank_current
	.globl  _enter_critical_section
	.globl  _leave_critical_section

	.area   _CODE

	; A = logical bank 0..79: 0..15 are physical banks 16..31, 16.. are
	; physical banks 48.. (32..47 hold the embedded files).
	; Preserves DE/HL/IX/IY; clobbers AF/BC.
__bank_map_next::
	push    de
	call    _enter_critical_section
	ld      (__bank_current),a
	add     a,#16
	cp      #32
	jr      c,.selected
	add     a,#16
.selected:
	ld      e,a
	ld      a,#0x56
	ld      bc,#0x243b
	out     (c),a
	inc     b
	ld      a,e
	add     a,a
	out     (c),a
	dec     b
	ld      a,#0x57
	out     (c),a
	inc     b
	ld      a,e
	add     a,a
	inc     a
	out     (c),a
	call    _leave_critical_section
	pop     de
	ret
