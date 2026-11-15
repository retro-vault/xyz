        ; Immutable drive parsing; preserve BC/DE/IX/IY and caller text.
        .module _esxdos_path_drive
        .optsdcc -mz80 sdcccall(1)
        .globl __zx_esx_path_drive
        .area _HEADER_DATA
__zx_esx_path_drive::
        push bc
        push hl
        ld c,a
        ld a,(hl)
        and #0xdf
        cp #'H'
        jp z,.hd
        sub #'A'
        cp #2
        jr nc,.default
        ld b,a
        inc hl
        ld a,(hl)
        cp #':'
        jp z,.accepted
.default:
        pop hl
        ld a,c
        pop bc
        ret
        .area _CODE
.hd:
        inc hl
        ld a,(hl)
        and #0xdf
        cp #'D'
        jp nz,.default
        inc hl
        ld a,(hl)
        sub #'0'
        cp #2
        jp nc,.default
        ld b,a
        inc hl
        ld a,(hl)
        cp #':'
        jp nz,.default
.accepted:
        inc hl
        pop af
        ld a,b
        rlca
        rlca
        rlca
        or #0x40
        pop bc
        ret
