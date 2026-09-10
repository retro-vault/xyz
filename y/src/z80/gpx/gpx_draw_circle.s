        ;; gpx_draw_circle.s
        ;;
        ;; Midpoint (Bresenham) circle outline.
        ;;
        ;; Portable: points use the register-fed __gpx_plot_raw core,
        ;; so clipping, blit modes and the framebuffer layout all stay the
        ;; backend's business and one copy of the code serves every
        ;; machine. The eight octant points of a step are emitted from a
        ;; single sign/swap selector. A step that lands on the 45-degree
        ;; diagonal emits only the first four: the other four are the same
        ;; pixels, and plotting them twice would cancel under BM_XOR.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-28   TS

        .module gpx_draw_circle
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_circle
        .globl  __gpx_plot_raw

        .globl  __gpx_circle_cmp
        .globl  __gpx_neg_hl

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_circle
        ;; Plot the circle centred on (x,y) with radius r. Every pixel is
        ;; plotted exactly once, so the outline is safe to draw in BM_XOR
        ;; and to erase by drawing it again. r == 0 is a single pixel and
        ;; a negative r draws nothing.
        ;;
        ;; Signature:
        ;;   void gpx_draw_circle(gpx_t *gpx, coord x, coord y, coord r,
        ;;                        color c, bmode m, const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = x
        ;;   stack: y(2), r(2), c(1), m(1), clip(2)
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY, and the alternate set
        ;;
        ;; References:
        ;;   __gpx_plot_raw
_gpx_draw_circle::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (10 bytes); xn/yn retain the shared comparison offsets
        ;; -1       octant selector: bit0 negates dx, bit1 dy, bit2 swaps
        ;; -2       how many selector values this step emits (4 or 8)
        ;; -3..-4   x (centre)
        ;; -5..-6   xn
        ;; -7..-8   yn
        ;; -9..-10  f     (decision variable)
        push    af                      ; selector/count slots
        push    de                      ; centre x: low at -4, high at -3
        ld      hl,#-6
        add     hl,sp
        ld      sp,hl

        ;; Pack color/mode once in the consumed color argument slot. RRA
        ;; takes exactly color bit 0, matching the public pixel wrappers.
        ld      a,8(ix)
        rra
        ld      a,9(ix)
        rla
        ld      8(ix),a

        ;; A negative radius draws nothing; r == 0 is one centre pixel.
        ld      a,7(ix)
        or      a
        jp      m,.dc_done
        or      6(ix)
        jr      nz,.dc_nsew
        ld      de,#0
        ld      h,d
        ld      l,e
        call    .plot_dxdy
        jp      .dc_done

.dc_nsew:
        ;; the four axis points, which the octant loop never reaches
        ld      de,#0
        ld      l,6(ix)
        ld      h,7(ix)
        call    .plot_dxdy              ; (x, y+r)
        ld      de,#0
        ld      l,6(ix)
        ld      h,7(ix)
        call    __gpx_neg_hl
        call    .plot_dxdy              ; (x, y-r)
        ld      hl,#0
        ld      e,6(ix)
        ld      d,7(ix)
        call    .plot_dxdy              ; (x+r, y)
        ld      hl,#0
        ld      e,6(ix)
        ld      d,7(ix)
        ex      de,hl
        call    __gpx_neg_hl
        ex      de,hl
        call    .plot_dxdy              ; (x-r, y)

        ;; xn = 0, yn = r
        xor     a
        ld      -5(ix),a
        ld      -6(ix),a
        ld      a,6(ix)
        ld      -7(ix),a
        ld      a,7(ix)
        ld      -8(ix),a

        ;; f = 1 - r
        ld      hl,#1
        ld      e,6(ix)
        ld      d,7(ix)
        or      a
        sbc     hl,de
        ld      -9(ix),l
        ld      -10(ix),h

.dc_loop:
        ;; xn < yn on entry: positive radius initially, then only the
        ;; eight-point case loops back. Do not compare twice per step.
        ;; xn++, f += 2*xn + 1
        ld      l,-5(ix)
        ld      h,-6(ix)
        inc     hl
        ld      -5(ix),l
        ld      -6(ix),h
        ;; ddx = 2*xn + 1; xn is already in HL.
        add     hl,hl
        inc     hl
        ex      de,hl
        ld      l,-9(ix)
        ld      h,-10(ix)
        add     hl,de
        ld      -9(ix),l
        ld      -10(ix),h

        ;; if (f >= 0) { yn--; f -= 2*yn; }
        bit     7,h
        jr      nz,.dc_stepped
        ld      l,-7(ix)
        ld      h,-8(ix)
        dec     hl
        ld      -7(ix),l
        ld      -8(ix),h
        ;; f -= 2*yn; yn is already in HL.
        add     hl,hl
        ex      de,hl
        ld      l,-9(ix)
        ld      h,-10(ix)
        or      a
        sbc     hl,de
        ld      -9(ix),l
        ld      -10(ix),h

.dc_stepped:
        ;; xn < yn emits all eight octant points, xn == yn only the four
        ;; distinct ones, xn > yn none at all
        call    __gpx_circle_cmp
        jp      m,.dc_eight
        ld      a,h
        or      l
        jp      nz,.dc_done
        ld      a,#4
        jr      .dc_emit
.dc_eight:
        ld      a,#8
.dc_emit:
        ld      -2(ix),a
        xor     a
        ld      -1(ix),a
.dc_emit_loop:
        call    .plot_sel
        ld      a,-1(ix)
        inc     a
        ld      -1(ix),a
        cp      -2(ix)
        jr      c,.dc_emit_loop
        cp      #8
        jp      z,.dc_loop

.dc_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: y(2), r(2), c(1), m(1), clip(2) = 8
        pop     hl                      ; return address
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        jp      (hl)


        ;; .plot_sel
        ;; plot one octant point for the selector in -1(ix)
        ;; clobbers: AF, BC, DE, HL
.plot_sel:
        ld      l,-5(ix)
        ld      h,-6(ix)                ; HL = dx magnitude (xn)
        ld      e,-7(ix)
        ld      d,-8(ix)                ; DE = dy magnitude (yn)
        bit     2,-1(ix)
        jr      z,.ps_noswap
        ex      de,hl                   ; mirrored octant
.ps_noswap:
        bit     0,-1(ix)
        jr      z,.ps_posx
        call    __gpx_neg_hl
.ps_posx:
        bit     1,-1(ix)
        jr      z,.ps_posy
        ex      de,hl
        call    __gpx_neg_hl
        ex      de,hl
.ps_posy:
        ex      de,hl                   ; DE = dx, HL = dy
        ;; falls into .plot_dxdy

        ;; .plot_dxdy
        ;; plot (x + dx, y + dy) with the caller's color, mode and clip
        ;; param: DE = dx, HL = dy
        ;; clobbers: AF, BC, DE, HL
.plot_dxdy:
        ld      c,-4(ix)
        ld      b,-3(ix)
        ex      de,hl
        add     hl,bc                   ; x + dx
        ex      de,hl                   ; DE = x + dx, HL = dy
        ld      c,4(ix)
        ld      b,5(ix)
        add     hl,bc                   ; HL = y + dy
        ld      c,10(ix)
        ld      b,11(ix)                ; BC = clip
        ld      a,8(ix)                 ; packed color/mode
        jp      __gpx_plot_raw          ; preserves the circle's IX frame
