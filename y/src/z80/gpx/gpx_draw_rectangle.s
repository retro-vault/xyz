        ;; gpx_draw_rectangle.s
        ;;
        ;; Rectangle outline renderer:
        ;;  - normalizes rectangle coordinates
        ;;  - top/bottom use requested line pattern
        ;;  - left/right sides are solid and exclude corners
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_draw_rectangle
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_rectangle
        .globl  __gpx_hline
        .globl  __gpx_bresenham_line
        .globl  __rect_cmp16s_lt
        .globl  __rect_unpack_norm

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_rectangle
        ;; Draw a rectangle outline, corners inclusive. All four sides are
        ;; axis-parallel, so each is emitted as a horizontal or vertical
        ;; run and the pattern is carried from one side to the next, which
        ;; keeps a dashed outline continuous around the corners.
        ;;
        ;; Signature:
        ;;   void gpx_draw_rectangle(gpx_t *gpx, rect_t *r,
        ;;                           color c, bmode m, uint8_t lpatt,
        ;;                           const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = r
        ;;   stack: c, m, lpatt, clip
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   _gpx_draw_line
_gpx_draw_rectangle::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (14 bytes)
        ;; -1..-2   x0
        ;; -3..-4   x1
        ;; -5..-6   y0
        ;; -7..-8   y1
        ;; -9..-10  (unused)
        ;; -11..-12 ytop = y0+1
        ;; -13..-14 ybot = y1-1
        ld      hl,#-14
        add     hl,sp
        ld      sp,hl

        ;; if (r == NULL) return
        ld      a,d
        or      e
        jp      z,.dr_done

        ;; unpack + normalize rect into locals
        call    __rect_unpack_norm

        ;; top edge: hline(x0..x1, y0)
        ld      l,7(ix)
        ld      h,8(ix)
        push    hl                      ; clip

        ld      a,6(ix)
        push    af
        inc     sp                      ; lpatt

        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; c,m
        push    hl                      ; y1 slot (ignored by hline)

        ld      l,-3(ix)
        ld      h,-4(ix)
        push    hl                      ; x1

        ld      l,-5(ix)
        ld      h,-6(ix)
        push    hl                      ; y0

        ld      e,-1(ix)                ; (gpx arg unused by hline)
        ld      d,-2(ix)
        call    __gpx_hline

        ;; A one-row rectangle has a single horizontal edge. Drawing it as
        ;; both the top and the bottom would apply the pattern twice, which
        ;; is invisible in BM_CPY but cancels under BM_XOR. The sides are
        ;; empty here too (ybot = y0-1 < ytop), so this is the whole rest.
        ld      a,-5(ix)
        cp      -7(ix)
        jr      nz,.dr_bottom
        ld      a,-6(ix)
        cp      -8(ix)
        jp      z,.dr_done

.dr_bottom:
        ;; bottom edge: hline(x0..x1, y1)
        ld      l,7(ix)
        ld      h,8(ix)
        push    hl                      ; clip

        ld      a,6(ix)
        push    af
        inc     sp                      ; lpatt

        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; c,m
        push    hl                      ; y1 slot (ignored by hline)

        ld      l,-3(ix)
        ld      h,-4(ix)
        push    hl                      ; x1

        ld      l,-7(ix)
        ld      h,-8(ix)
        push    hl                      ; y0

        ld      e,-1(ix)
        ld      d,-2(ix)
        call    __gpx_hline

        ;; ytop = y0 + 1
        ld      l,-5(ix)
        ld      h,-6(ix)
        inc     hl
        ld      -11(ix),l
        ld      -12(ix),h

        ;; ybot = y1 - 1
        ld      l,-7(ix)
        ld      h,-8(ix)
        dec     hl
        ld      -13(ix),l
        ld      -14(ix),h

        ;; if (ybot < ytop) no side pixels
        ld      l,-13(ix)
        ld      h,-14(ix)
        ld      e,-11(ix)
        ld      d,-12(ix)
        call    __rect_cmp16s_lt
        jr      c,.dr_done

        ;; left side: vline(x0, ytop..ybot), solid
        ld      l,7(ix)
        ld      h,8(ix)
        push    hl                      ; clip

        ld      a,#0xff
        push    af
        inc     sp                      ; lpatt solid

        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; c,m

        ld      l,-13(ix)
        ld      h,-14(ix)
        push    hl                      ; y1

        ld      e,-1(ix)
        ld      d,-2(ix)                ; DE = x0 (kept for the call)
        push    de                      ; x1 slot (same as x0)

        ld      l,-11(ix)
        ld      h,-12(ix)
        push    hl                      ; y0

        call    __gpx_bresenham_line    ; dx=0, downward: bres fast path

        ;; A one-column rectangle has a single vertical edge, for the same
        ;; reason the one-row case above has a single horizontal one.
        ld      a,-1(ix)
        cp      -3(ix)
        jr      nz,.dr_right
        ld      a,-2(ix)
        cp      -4(ix)
        jr      z,.dr_done

.dr_right:
        ;; right side: vline(x1, ytop..ybot), solid
        ld      l,7(ix)
        ld      h,8(ix)
        push    hl                      ; clip

        ld      a,#0xff
        push    af
        inc     sp                      ; lpatt solid

        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; c,m

        ld      l,-13(ix)
        ld      h,-14(ix)
        push    hl                      ; y1

        ld      e,-3(ix)
        ld      d,-4(ix)                ; DE = x1 (kept for the call)
        push    de                      ; x1 slot

        ld      l,-11(ix)
        ld      h,-12(ix)
        push    hl                      ; y0

        call    __gpx_bresenham_line    ; dx=0, downward: bres fast path

.dr_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: c(1), m(1), lpatt(1), clip(2) = 5
        pop     de
        pop     hl
        pop     hl
        inc     sp
        push    de
        ret
