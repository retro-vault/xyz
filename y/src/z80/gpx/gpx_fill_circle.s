        ;; gpx_fill_circle.s
        ;;
        ;; Filled circle, walked by the same midpoint stepper that draws
        ;; the outline.
        ;;
        ;; Portable: each scanline of the disc is a horizontal
        ;; gpx_draw_line, so the backend's own run renderer does the work
        ;; -- a byte-span writer on the ZX and the CPC, the EF9367's vector
        ;; generator on the Partner -- and one copy of the code serves
        ;; every machine. Every row is emitted exactly once, which keeps
        ;; BM_XOR meaningful, and the row spans are the ones
        ;; gpx_draw_circle's outline lands on, so a filled circle and an
        ;; outline of the same radius agree pixel for pixel.
        ;;
        ;; The fill pattern is anchored to the circle's bounding box, not
        ;; to each row: row n takes fpatt[n % fpatt_len] counting from the
        ;; top of the box, and the byte is turned so its MSB still falls on
        ;; the box's left edge. Filling a circle and filling its bounding
        ;; box therefore lay down the same bits. A fill pattern runs
        ;; MSB-first from the left edge while a line consumes its pattern
        ;; LSB-first from its own start, so each row's byte is reversed and
        ;; then rotated -- the same conversion the Partner's rectangle fill
        ;; does before it hands a row to the same renderer.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-28   TS

        .module gpx_fill_circle
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_fill_circle
        .globl  _gpx_draw_line

        .globl  __gpx_circle_cmp
        .globl  __ret_clean11
        .globl  __gpx_neg_hl

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_fill_circle
        ;; Fill the disc centred on (x,y) with radius r. r == 0 is a
        ;; single pixel, a negative r or an empty pattern draws nothing.
        ;;
        ;; Signature:
        ;;   void gpx_fill_circle(gpx_t *gpx, coord x, coord y, coord r,
        ;;                        color c, bmode m,
        ;;                        uint8_t *fpatt, uint8_t fpatt_len,
        ;;                        const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = x
        ;;   stack: y(2), r(2), c(1), m(1), fpatt(2), fpatt_len(1), clip(2)
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY, and the alternate set
        ;;
        ;; References:
        ;;   _gpx_draw_line
_gpx_fill_circle::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (12 bytes)
        ;; -1..-2   dy of the row being drawn
        ;; -3..-4   x (centre)
        ;; -5..-6   xn
        ;; -7..-8   yn
        ;; -9..-10  f     (decision variable)
        ;; -11..-12 half width of that row
        push    af                      ; dy slots
        push    de                      ; centre x: low at -4, high at -3
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl

        ;; a negative radius or an empty pattern draws nothing
        ld      a,7(ix)
        or      a
        jp      m,.fc_done
        ld      a,12(ix)
        or      a
        jp      z,.fc_done

        ;; the centre row spans the full diameter
        ld      hl,#0
        ld      e,6(ix)
        ld      d,7(ix)
        call    .row

        ;; r == 0 was that single row
        ld      a,6(ix)
        or      7(ix)
        jp      z,.fc_done

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

.fc_loop:
        ;; The positive radius and the previous step establish xn < yn.
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

        ;; f >= 0 finishes the pair of rows at +/-yn: they are as wide as
        ;; they will get, which is the xn from before this step
        bit     7,h
        jr      nz,.fc_stepped
        ld      l,-7(ix)
        ld      h,-8(ix)                ; dy = yn
        ld      e,-5(ix)
        ld      d,-6(ix)
        dec     de                      ; w = xn - 1
        call    .row_pair
        ld      l,-7(ix)
        ld      h,-8(ix)
        dec     hl
        ld      -7(ix),l
        ld      -8(ix),h                ; yn--
        ;; f -= 2*yn; yn is already in HL.
        add     hl,hl
        ex      de,hl
        ld      l,-9(ix)
        ld      h,-10(ix)
        or      a
        sbc     hl,de
        ld      -9(ix),l
        ld      -10(ix),h               ; f -= 2*yn

.fc_stepped:
        ;; the pair of rows at +/-xn is final as soon as xn is, as long as
        ;; the walk has not passed the diagonal
        call    __gpx_circle_cmp
        jp      m,.fc_row_xn
        ld      a,h
        or      l
        jp      nz,.fc_done
.fc_row_xn:
        ld      l,-5(ix)
        ld      h,-6(ix)                ; dy = xn
        ld      e,-7(ix)
        ld      d,-8(ix)                ; w = yn
        push    af                      ; preserve the comparison through draw
        call    .row_pair
        pop     af
        jp      m,.fc_loop              ; equality was the final pair

.fc_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: y(2), r(2), c(1), m(1), fpatt(2), fpatt_len(1),
        ;; clip(2) = 11
        jp      __ret_clean11


        ;; .row_pair
        ;; draw the two rows dy above and below the centre
        ;; param: HL = dy (> 0), DE = half width
        ;; clobbers: AF, BC, DE, HL
.row_pair:
        call    .row
        ld      l,-1(ix)
        ld      h,-2(ix)
        call    __gpx_neg_hl
        ld      e,-11(ix)
        ld      d,-12(ix)
        jp      .row

        ;; .row
        ;; draw one row of the disc as a horizontal line
        ;; param: HL = dy from the centre, DE = half width
        ;; clobbers: AF, BC, DE, HL
.row:
        ld      -1(ix),l
        ld      -2(ix),h
        ld      -11(ix),e
        ld      -12(ix),d

        ;; the row's byte, counted from the top of the bounding box:
        ;; fpatt[(dy + r) % fpatt_len]
        ld      e,6(ix)
        ld      d,7(ix)
        add     hl,de                   ; dy + r, never negative
        ld      a,12(ix)
        ld      c,a
        dec     a
        and     c
        jr      nz,.row_divide          ; not a power of two: divide
        ld      a,c
        dec     a
        and     l
        jr      .row_have_idx
.row_divide:
        call    .mod16_8
.row_have_idx:
        ld      e,a
        ld      d,#0
        ld      l,10(ix)
        ld      h,11(ix)
        add     hl,de
        ld      a,(hl)
        or      a
        ret     z                       ; an empty pattern row draws nothing
        cp      #0xFF
        jr      z,.row_draw             ; solid rows need no phase conversion

        ;; Reverse once, then rotate by (r - width) modulo eight. Only the
        ;; low coordinate bytes contribute to this phase.
        ld      b,#8
.row_reverse:
        rlca
        rr      c
        djnz    .row_reverse
        ld      a,6(ix)
        sub     -11(ix)
        and     #0x07
        ld      b,a
        ld      a,c
        jr      z,.row_draw
.row_rotate:
        rrca
        djnz    .row_rotate

.row_draw:
        ;; All backend line entries preserve IX and ignore the context
        ;; argument. Keep the shape frame live across their callee cleanup.
        ld      l,13(ix)
        ld      h,14(ix)
        push    hl                      ; clip
        push    af
        inc     sp                      ; lpatt
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl                      ; c, m
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      e,-11(ix)
        ld      d,-12(ix)
        add     hl,de
        ex      de,hl                   ; DE = x + w
        ld      l,-1(ix)
        ld      h,-2(ix)
        ld      c,4(ix)
        ld      b,5(ix)
        add     hl,bc                   ; HL = y + dy
        push    hl                      ; y1
        push    de                      ; x1
        push    hl                      ; y0
        ld      l,-11(ix)
        ld      h,-12(ix)
        add     hl,hl                   ; DE still holds x+w: subtract 2*w
        ex      de,hl
        or      a
        sbc     hl,de
        ex      de,hl                   ; DE = x - w
        call    _gpx_draw_line
        ret

        ;; .mod16_8
        ;; param: HL = dividend (>= 0), C = divisor (>= 1)
        ;; return: A = HL % C
        ;; clobbers: AF, B, HL
.mod16_8:
        xor     a
        ld      b,#16
.md_loop:
        add     hl,hl
        rla
        jr      c,.md_subtract
        cp      c
        jr      c,.md_next
.md_subtract:
        sub     c
.md_next:
        djnz    .md_loop
        ret
