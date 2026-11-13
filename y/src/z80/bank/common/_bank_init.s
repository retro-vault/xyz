        .globl __bank_model, __sys_heap, __os_malloc, __os_free
	; Initialize every logical 16 KiB executable-bank arena.
	;
	; MIT License (see: LICENSE)
	; Copyright (C) 2026 tomaz stih

	.module _bank_init
	.optsdcc -mz80 sdcccall(1)

	.globl  __bank_init
	.globl  __bank_map
	.globl  __bank_count
	.globl  _mem_init

	.area   _CODE

	; Initializes C000h..FFFFh in every configured bank as one heap.
	; The routine restores logical bank zero. Clobbers AF/DE/HL.
__bank_init::
        .if YOS_UNIFIED_48K
        ld a,(__bank_model)
        or a
        jr nz,.banked
        ld (__bank_count),a
        ld hl,#0xfff7
        ld de,#__sys_heap
        sbc hl,de
        ld (__sys_heap+5),hl
        ret
.banked:
        .endif
	xor     a
.next:
	push    af
	call    __bank_map
	ld      de,#0x4000
	ld      hl,#0xc000
	call    _mem_init
	pop     af
	inc     a
	ld      hl,#__bank_count
	cp      (hl)
	jr      c,.next
	xor     a
	jp      __bank_map
