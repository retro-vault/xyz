        .module _gpx_hline_next_l2
        .optsdcc -mz80 sdcccall(1)
        .globl __gpx_hline
        .globl __gpx_plot_raw
        .globl __ret_clean11
        .area _CODE
;; Fallback horizontal line.  Arguments match gpx_draw_line's hline tail path;
;; it deliberately uses the pixel primitive so bank boundaries are harmless.
__gpx_hline::
        push ix
        ld ix,#0
        add ix,sp
        ;; x0 is DE, y0 at 4(ix), x1 at 6(ix), then color/mode/pattern/clip.
        ld l,4(ix)
        ld h,5(ix)
        ld a,6(ix)
        ld c,a
        ld a,7(ix)
        ld b,a
        ;; Normalize/reject reversed spans.
        ld a,c
        sub e
        ld a,b
        sbc a,d
        jr c,.done
        ld a,8(ix)
        ld b,a
        ld a,9(ix)
        rla
        and #3
        push de
        push hl
        push bc
        call __gpx_plot_raw
        pop bc
        pop hl
        pop de
        inc de
        ld a,e
        cp c
        jr nz,.loop
        ld a,d
        cp b
        jr nz,.loop
.done:
        ld a,10(ix)
        pop ix
        jp __ret_clean11
.loop:
        jr .loop
