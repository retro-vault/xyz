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

	; input: A = logical bank 0..125. Physical 16 KiB pages 2 and 5 back
	; fixed RAM below C000h, so the logical map skips both. The selected
	; physical page pair is installed in slots 6 and 7.
	; Preserves DE/HL/IX/IY; clobbers AF/BC.
__bank_map_next::
	push    de
	call    _enter_critical_section
	ld      (__bank_current),a
	ld      e,a
	cp      #2
	jr      c,.selected
	inc     e                       ; skip physical 16 KiB page 2
	cp      #4
	jr      c,.selected
	inc     e                       ; skip physical 16 KiB page 5
.selected:
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
