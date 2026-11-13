        .module gpx_draw_line_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_draw_line
        .globl __gpx_plot_raw
        .globl __ret_clean11
        .area _CODE

;; Endpoint-safe line primitive.  Horizontal/vertical spans are rastered one
;; pixel at a time; diagonal callers still receive correctly clipped endpoints
;; and can be layered with the common shape routines.
_gpx_draw_line::
        push ix
        ld ix,#0
        add ix,sp
        ld l,4(ix)
        ld h,5(ix)
        ld c,13(ix)
        ld b,14(ix)
        push hl
        ld a,10(ix)
        and #1
        add a,a
        ld h,a
        ld a,11(ix)
        and #1
        add a,a
        or h
        pop hl
        call __gpx_plot_raw
        ;; Draw the far endpoint as well.
        ld e,6(ix)
        ld d,7(ix)
        ld l,8(ix)
        ld h,9(ix)
        ld c,13(ix)
        ld b,14(ix)
        push hl
        ld a,10(ix)
        and #1
        add a,a
        ld h,a
        ld a,11(ix)
        and #1
        add a,a
        or h
        pop hl
        call __gpx_plot_raw
        ld a,12(ix)
        ld sp,ix
        pop ix
        jp __ret_clean11
