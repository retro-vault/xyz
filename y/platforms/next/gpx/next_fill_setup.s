        ;; E=rows (zero means 256). Convert a column before enabling the Layer 2 overlay.
        .module next_fill_setup
        .optsdcc -mz80 sdcccall(1)
        .globl __next_fill_rows, fi_flags, fi_mask, fill_rows
        .area _CODE
__next_fill_rows::
        ld a,(fi_flags)
        and #1
        ld c,#0xff
        jr z,.fi_ink
        ld c,#0
.fi_ink:
        ld a,c
        cpl
        ld b,a
        ld a,(fi_mask)
        ld d,a
        ld hl,#fill_rows
.fi_conv:
        ld a,(hl)
        and d
        ld a,b
        jr z,.fi_pixel
        ld a,c
.fi_pixel:
        ld (hl),a
        inc hl
        dec e
        jr nz,.fi_conv
        ret
