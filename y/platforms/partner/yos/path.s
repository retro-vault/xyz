        ;; ALTO A:/B: names and native fd0:/hd0: names share one filesystem.
        ;; A: boot floppy, B: hard disk. The shared PRC tree is flat on CP/M.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_yos_path
        .globl __partner_path
        .area _BSS
.path: .ds 32
.base: .ds 2
        .area _CODE
        ;; HL=input -> HL=bounded, normalized path; A=0/FF. Serialized.
__partner_path::
        push de
        push bc
        ld de,#.path
        ld a,(hl)
        and #0xdf
        ld b,a
        inc hl
        ld a,(hl)
        dec hl
        cp #58
        jr nz,.native
        ld a,b
        cp #65
        jr z,.floppy
        cp #66
        jr nz,.bad
        ld a,#104
        jr .device
.floppy:
        ld a,#102
.device:
        ld (de),a
        inc de
        ld a,#100
        ld (de),a
        inc de
        ld a,#48
        ld (de),a
        inc de
        ld a,#58
        ld (de),a
        inc de
        ld a,#47
        ld (de),a
        inc de
        inc hl
        inc hl
        ;; Numeric first components are CP/M user areas. Preserve these
        ;; for libcpmfs to validate; legacy ALTO/service paths stay flat.
        ld a,(hl)
        cp #47
        jr nz,.area_check
        inc hl
.area_check:
        ld a,(hl)
        sub #48
        cp #10
        jr nc,.legacy
        ld b,#26
        jr .copy
.legacy:
        ld (.base),de
        ld b,#26
.flat:
        ld a,(hl)
        inc hl
        cp #47
        jr nz,.flat_char
        ld de,(.base)
        ld b,#26
        jr .flat
.flat_char:
        ld (de),a
        inc de
        or a
        jr z,.ok
        djnz .flat
        jr .bad
.native:
        ld b,#32
.copy:
        ld a,(hl)
        inc hl
        ld (de),a
        inc de
        or a
        jr z,.ok
        djnz .copy
.bad:
        ld a,#255
        jr .done
.ok:
        ld hl,#.path
        xor a
.done:
        pop bc
        pop de
        ret
