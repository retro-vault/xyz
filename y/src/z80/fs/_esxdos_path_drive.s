        ; Translate the optional YOS physical-drive prefix for esxDOS.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        .module _esxdos_path_drive
        .optsdcc -mz80 sdcccall(1)
        .globl __zx_esx_path_drive
        .area _CODE
        ; Input: HL = validated path, A = fallback native drive.
        ; Output: HL skips A:/B: prefix, A = 0x40/0x48; otherwise unchanged.
        ; Clobbers flags; preserves BC, DE, IX, IY. No stack arguments.
__zx_esx_path_drive::
        push bc
        ld c,a
        ld a,(hl)
        and #0xdf
        sub #'A'
        cp #2
        jr nc,.done
        rlca
        rlca
        rlca
        or #0x40
        ld b,a
        inc hl
        ld a,(hl)
        cp #':'
        dec hl
        jr nz,.done
        inc hl
        inc hl
        ld c,b
.done:
        ld a,c
        pop bc
        ret
