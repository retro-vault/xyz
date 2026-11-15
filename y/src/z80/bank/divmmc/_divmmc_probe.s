        ; Reversible alias test for 4/8/16/32/64 physical 8 KiB SRAM pages.
        ; Inputs: running in fixed RAM, interrupts disabled. Outputs: B=user
        ; pages (five firmware pages excluded). Clobbers AF/BC/DE/HL.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_probe
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_probe
        .area _CODE
__divmmc_probe::
        ld hl,#0x3fff
        ld a,#0x80
        out (0xe3),a
        ld d,(hl)
        ld a,d
        cpl
        ld (hl),a
        cp (hl)                       ; no SRAM / no E3 decoder is not 512 KiB
        ld (hl),d
        ld b,#0
        jr nz,.absent
        ld b,#4
.next:
        ld a,b
        or #0x80
        out (0xe3),a
        ld a,(hl)
        cpl
        ld c,a
        ld a,#0x80
        out (0xe3),a
        ld (hl),c
        ld a,b
        or #0x80
        out (0xe3),a
        ld a,(hl)
        cp c
        ld a,#0x80
        out (0xe3),a
        ld (hl),d
        jr z,.found
        sla b
        bit 6,b
        jr z,.next
.found:
        xor a
        out (0xe3),a                   ; do not enable sticky MAPRAM while probing
        ld a,b
        sub #5
        jr nc,.usable
        xor a
.usable:
        ld b,a
        ret
.absent:
        xor a
        out (0xe3),a
        ret
