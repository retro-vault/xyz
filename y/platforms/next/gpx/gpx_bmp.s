        .module gpx_bmp_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_draw_bmp
        .globl _gpx_draw_bmp_clip
        .globl __gpx_plot_raw
        .area _CODE

_gpx_draw_bmp::
        ld b,#0
        ld c,#1
        jp .entry
_gpx_draw_bmp_clip::
        ;; B=blit mode, C=color
.entry:
        push ix
        ld ix,#0
        add ix,sp
        ld hl,#-16
        add hl,sp
        ld sp,hl
        ;; locals: x0 -2, y0 -4, payload -6, width -8, height -9,
        ;; stride -10, rowptr -12, row -13, col -14, flags -16
        ld -2(ix),e
        ld -1(ix),d
        ld l,4(ix)
        ld h,5(ix)
        ld -4(ix),l
        ld -3(ix),h
        ld -16(ix),b
        ld -15(ix),c
        ld l,6(ix)
        ld h,7(ix)
        ld a,(hl)
        and #0x0f
        inc a
        ld -10(ix),a
        inc hl
        ld a,(hl)
        ld -8(ix),a
        inc hl
        ld a,(hl)
        ld -9(ix),a
        inc hl
        inc hl
        inc hl
        ld -6(ix),l
        ld -5(ix),h
        xor a
        ld -13(ix),a
.row:
        ld a,-13(ix)
        cp -9(ix)
        jp nc,.done
        ;; rowptr = payload + row*stride
        ld l,-6(ix)
        ld h,-5(ix)
        ld a,-13(ix)
        ld b,a
        ld a,-10(ix)
        ld e,a
        ld d,#0
.rowadd:
        ld a,b
        or a
        jr z,.rowready
        add hl,de
        dec b
        jr .rowadd
.rowready:
        ld -12(ix),l
        ld -11(ix),h
        xor a
        ld -14(ix),a
.col:
        ld a,-14(ix)
        cp -8(ix)
        jp nc,.nextrow
        ;; Test source bit MSB first.
        ld e,a
        srl e
        srl e
        srl e
        ld l,-12(ix)
        ld h,-11(ix)
        ld d,#0
        add hl,de
        ld a,(hl)
        ld e,-14(ix)
        and #7
        ld e,a
        ld a,#0x80
        jr z,.mask
.maskshift:
        srl a
        dec e
        jr nz,.maskshift
.mask:
        ld e,a
        ld a,(hl)
        and e
        jr z,.skip
        ;; Destination point and packed flags.
        ld a,-14(ix)
        add a,-2(ix)
        ld e,a
        ld a,-1(ix)
        adc a,#0
        ld d,a
        ld a,-13(ix)
        add a,-4(ix)
        ld l,a
        ld a,-3(ix)
        adc a,#0
        ld h,a
        push de
        push hl
        ld c,8(ix)
        ld b,9(ix)
        ld a,-16(ix)
        and #1
        add a,a
        ld e,a
        ld a,-15(ix)
        and #1
        or e
        pop hl
        pop de
        call __gpx_plot_raw
.skip:
        inc -14(ix)
        jp .col
.nextrow:
        inc -13(ix)
        jp .row
.done:
        ld sp,ix
        pop ix
        ret
