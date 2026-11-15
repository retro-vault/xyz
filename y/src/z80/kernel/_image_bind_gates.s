        ; Bind an explicitly described XPRG restart-gate table to fixed RAM.
        ; IX=loader frame, header +36=code offset, +38=number of JP slots.
        ; A=0 success / 4 malformed or unsupported image. Preserves IX/IY.
        .module image_bind_gates
        .optsdcc -mz80 sdcccall(1)
        .globl __image_bind_gates, __sys_vec_tbl
        .area _CODE
__image_bind_gates::
        ld a,38(ix)
        cp #4
        jr nz,.bad
        ld a,39(ix)
        or a
        jr nz,.bad
        ld l,36(ix)
        ld h,37(ix)
        ld de,#12
        add hl,de
        jr c,.bad
        ld e,94(ix)
        ld d,95(ix)
        or a
        sbc hl,de
        jr c,.valid
        jr nz,.bad
.valid:
        ld l,36(ix)
        ld h,37(ix)
        ld e,68(ix)
        ld d,69(ix)
        add hl,de
        ld de,#__sys_vec_tbl+6
        ld b,#4
        ld c,#0x18
.slot:
        ld a,(hl)
        cp #0xc3
        jr nz,.bad
        inc hl
        ld a,(hl)
        cp c
        jr nz,.bad
        inc hl
        ld a,(hl)
        or a
        jr nz,.bad
        ld (hl),d
        dec hl
        ld (hl),e
        inc hl
        inc hl
        inc de
        inc de
        inc de
        ld a,c
        add a,#8
        ld c,a
        djnz .slot
        xor a
        ret
.bad:
        ld a,#4
        ret
