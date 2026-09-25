	; Spectrum 128K 7FFD logical-bank backend.
	;
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih

	.module _bank_map_128
	.optsdcc -mz80 sdcccall(1)

	.globl  __bank_map_128
	.globl  __bank_current
	.globl  _enter_critical_section
	.globl  _leave_critical_section

	.area   _CODE

	; input: A = logical bank 0..5
	; Logical banks select physical pages 0,1,3,4,6,7. Pages 5 and 2 stay
	; permanently visible below C000h. YOS occupies the 48 BASIC ROM slot used
	; by ESXIDE on 128K hardware, so every paging write keeps ROM-select bit 4
	; set while leaving the screen and paging-disable bits clear. Preserves
	; DE/HL/IX/IY.
__bank_map_128::
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
	ld      a,e
	or      #0x10                    ; retain the YOS/48-ROM slot
	ld      bc,#0x7ffd
	out     (c),a
	call    _leave_critical_section
	pop     de
	ret
