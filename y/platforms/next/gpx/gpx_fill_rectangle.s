        .module gpx_fill_rectangle_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_fill_rectangle
        .globl __gpx_plot_raw
        .area _CODE

;; Pixel fallback rectangle fill.  It intentionally keeps the pattern phase
;; simple (MSB first, anchored at the rectangle origin) and lets the raw plot
;; routine handle Layer 2 bank crossings and clipping.
_gpx_fill_rectangle::
        push ix
        ld ix,#0
        add ix,sp
        ld hl,#-12
        add hl,sp
        ld sp,hl
        ;; locals -2 x0, -4 y0, -6 x1, -8 y1, -10 x, -12 y
        ld a,(de)
        ld l,a
        inc de
        ld a,(de)
        ld h,a
        ld -2(ix),l
        ld -1(ix),h
        inc de
        ld a,(de)
        ld l,a
        inc de
        ld a,(de)
        ld h,a
        ld -4(ix),l
        ld -3(ix),h
        inc de
        ld a,(de)
        ld l,a
        inc de
        ld a,(de)
        ld h,a
        ld -6(ix),l
        ld -5(ix),h
        inc de
        ld a,(de)
        ld l,a
        inc de
        ld a,(de)
        ld h,a
        ld -8(ix),l
        ld -7(ix),h
        ;; normalize x/y when both coordinates are in the visible range
        ld a,-2(ix)
        cp -6(ix)
        jr c,.xok
        jr z,.xok
        ld a,-2(ix)
        ld l,-6(ix)
        ld -2(ix),l
        ld -6(ix),a
.xok:
        ld a,-4(ix)
        cp -8(ix)
        jr c,.yok
        jr z,.yok
        ld a,-4(ix)
        ld l,-8(ix)
        ld -4(ix),l
        ld -8(ix),a
.yok:
        ld l,-4(ix)
        ld h,-3(ix)
        ld -12(ix),l
        ld -11(ix),h
.row:
        ld l,-2(ix)
        ld h,-1(ix)
        ld -10(ix),l
        ld -9(ix),h
.col:
        ld e,-10(ix)
        ld d,-9(ix)
        ld l,-12(ix)
        ld h,-11(ix)
        ld c,9(ix)
        ld b,10(ix)
        push hl
        ld a,4(ix)
        and #1
        ld h,a
        ld a,5(ix)
        and #1
        add a,a
        or h
        pop hl
        call __gpx_plot_raw
        inc -10(ix)
        jr nz,.x_test
        inc -9(ix)
.x_test:
        ld a,-10(ix)
        cp -6(ix)
        jr nz,.col
        ld a,-9(ix)
        cp -5(ix)
        jr nz,.col
        inc -12(ix)
        jr nz,.y_test
        inc -11(ix)
.y_test:
        ld a,-12(ix)
        cp -8(ix)
        jr nz,.row
        ld a,-11(ix)
        cp -7(ix)
        jr nz,.row
        ld sp,ix
        pop ix
        ret
