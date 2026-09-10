        ;; gpx_draw_polygon.s
        ;;
        ;; Closed polygon outline.
        ;;
        ;; Portable: every edge is a gpx_draw_line, so clipping, blit modes
        ;; and the framebuffer layout stay the backend's business and one
        ;; copy of the code serves every machine. The rotated pattern each
        ;; line returns is carried into the next edge, the way
        ;; gpx_draw_rectangle carries it around a rectangle, so a dashed
        ;; outline stays continuous around the corners.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-28   TS

        .module gpx_draw_polygon
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_polygon
        .globl  _gpx_draw_line

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_polygon
        ;; Draw the closed path through n points. Fewer than two points
        ;; draws nothing.
        ;;
        ;; Signature:
        ;;   void gpx_draw_polygon(gpx_t *gpx, point_t *pts, uint8_t n,
        ;;                         color c, bmode m, uint8_t lpatt,
        ;;                         const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = pts
        ;;   stack: n(1), c(1), m(1), lpatt(1), clip(2)
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY, and the alternate set
        ;;
        ;; References:
        ;;   _gpx_draw_line
_gpx_draw_polygon::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (8 bytes)
        ;; -1..-2   pts
        ;; -3..-4   &pts[i]
        ;; -5..-6   &pts[j], j = (i + 1) % n
        ;; -7       edges left to draw
        ;; -8       the pattern, rotated on by every edge drawn
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl
        ld      -1(ix),e
        ld      -2(ix),d

        ;; a closed path needs two points
        ld      a,4(ix)
        cp      #2
        jp      c,.dp_done

        ;; edges left = n, &pts[0], &pts[1]
        ld      -7(ix),a
        ld      -3(ix),e
        ld      -4(ix),d
        ld      hl,#4
        add     hl,de
        ld      -5(ix),l
        ld      -6(ix),h

        ld      a,7(ix)
        ld      -8(ix),a                ; starting pattern

.dp_edge:
        ;; gpx_draw_line(gpx, pts[i].x, pts[i].y, pts[j].x, pts[j].y,
        ;;               c, m, patt, clip). Backend line entries preserve
        ;; IX, ignore the context argument and clean their own arguments.
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl                      ; clip
        ld      a,-8(ix)
        push    af
        inc     sp                      ; lpatt
        ld      l,5(ix)
        ld      h,6(ix)
        push    hl                      ; c, m

        ld      l,-5(ix)
        ld      h,-6(ix)                ; &pts[j]
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        push    bc                      ; y1
        push    de                      ; x1

        ld      l,-3(ix)
        ld      h,-4(ix)                ; &pts[i]
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; DE = x0
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        push    bc                      ; y0
        call    _gpx_draw_line
        ld      -8(ix),a                ; carry the phase into the next edge

        ;; The next edge starts at the endpoint just consumed. Only the
        ;; final endpoint wraps; the point count also handles n == 255.
        dec     -7(ix)
        jr      z,.dp_done
        ld      l,-5(ix)
        ld      h,-6(ix)
        ld      -3(ix),l
        ld      -4(ix),h
        ld      a,-7(ix)
        dec     a
        jr      nz,.dp_next_j
        ld      l,-1(ix)
        ld      h,-2(ix)
        jr      .dp_store_j
.dp_next_j:
        ld      de,#4
        add     hl,de
.dp_store_j:
        ld      -5(ix),l
        ld      -6(ix),h
        jp      .dp_edge

.dp_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: n(1), c(1), m(1), lpatt(1), clip(2) = 6
        pop     hl
        pop     bc
        pop     bc
        pop     bc
        jp      (hl)
