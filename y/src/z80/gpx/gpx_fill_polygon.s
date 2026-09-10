        ;; gpx_fill_polygon.s
        ;;
        ;; Scanline polygon fill, even-odd rule.
        ;;
        ;; Portable: each span uses the internal horizontal line entry, so
        ;; the backend's own run renderer does the work -- a byte-span writer on
        ;; the ZX and the CPC, the EF9367's vector generator on the Partner
        ;; -- and one copy of the code serves every machine.
        ;;
        ;; One walker per non-horizontal edge steps the same Bresenham
        ;; gpx_draw_line uses, so a span ends exactly where the outline of
        ;; the same polygon is drawn. An edge the polygon traverses upwards
        ;; has to be walked downwards here, and this rasterizer is not
        ;; symmetric under reversal, so a flipped edge takes the opposite
        ;; rounding bias -- (dx-1)/2 rather than dx/2 -- which reproduces
        ;; the reversed walk exactly.
        ;;
        ;; Crossings use the half-open rule y0 <= y < y1, so a shared vertex
        ;; counts once; on the polygon's own bottom row the rule closes, so
        ;; the bottom edge is filled the way gpx_fill_rectangle fills its
        ;; bottom row. Spans on a row are then clamped to be disjoint,
        ;; because two of them can meet on a shared vertex pixel and BM_XOR
        ;; would cancel it.
        ;;
        ;; The fill pattern is anchored to the polygon's bounding box: row n
        ;; takes fpatt[n % fpatt_len] counting from the top of the box, and
        ;; the byte is reversed and rotated so its MSB falls on the box's
        ;; left edge -- the same conversion the circle fill does.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-28   TS

        .module gpx_fill_polygon
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_fill_polygon
        .globl  __gpx_hline

        ;; Raising MAXPTS costs ERECSZ bytes of stack per point plus four
        ;; for its crossing slot. Keep it in step with GPX_MAX_POLY_PTS.
        .equ    MAXPTS, 12
        .equ    ERECSZ, 13
        .equ    EDGEBYTES, MAXPTS * ERECSZ
        .equ    CROSSBYTES, MAXPTS * 4
        .equ    SCALARS, 34
        .equ    FRAME, SCALARS + EDGEBYTES + CROSSBYTES

        ;; edge record fields
        .equ    E_Y1, 0
        .equ    E_DX, 2
        .equ    E_DY, 4
        .equ    E_ERR, 6
        .equ    E_CX, 8
        .equ    E_Y0, 10
        .equ    E_SX, 12

        .globl  __gpx_neg_hl

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_fill_polygon
        ;; Fill the closed path through n points with a repeating pattern.
        ;; Fewer than three points, more than MAXPTS of them, or an empty
        ;; pattern draws nothing.
        ;;
        ;; Signature:
        ;;   void gpx_fill_polygon(gpx_t *gpx, point_t *pts, uint8_t n,
        ;;                         color c, bmode m,
        ;;                         uint8_t *fpatt, uint8_t fpatt_len,
        ;;                         const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = pts
        ;;   stack: n(1), c(1), m(1), fpatt(2), fpatt_len(1), clip(2)
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY, and the alternate set
        ;;
        ;; References:
        ;;   __gpx_hline
_gpx_fill_polygon::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals. The scalars stay inside IX's one-byte reach; the two
        ;; tables below them are walked with cursors instead.
        ;; -1..-2   next free edge record while building
        ;; -3..-4   pts while building; crossing cursor while drawing
        ;; -5       nedges
        ;; -6       nints, the crossings on this row
        ;; -7..-8   ymin
        ;; -9..-10  ymax
        ;; -11..-12 xmin
        ;; -13..-14 y, the current scanline
        ;; -15      patt_idx
        ;; -16      set on the polygon's bottom row
        ;; -17..-18 &pts[i]
        ;; -19..-20 &pts[j]
        ;; -21..-22 pi.x while building; crossing cursor while drawing
        ;; -23..-24 pi.y while building; span x0 while drawing
        ;; -25..-26 pj.x while building; previous span end while drawing
        ;; -27..-28 pj.y while building; span x1 while drawing
        ;; -29      counter
        ;; -30      this row's pattern byte
        ;; -31..-32 edge table base
        ;; -33..-34 crossing table base
        ld      hl,#-FRAME
        add     hl,sp
        ld      sp,hl
        ld      -3(ix),e
        ld      -4(ix),d

        ;; table bases
        push    ix
        pop     hl
        ld      de,#-SCALARS-EDGEBYTES
        add     hl,de
        ld      -31(ix),l
        ld      -32(ix),h
        ld      -1(ix),l
        ld      -2(ix),h
        ld      de,#-CROSSBYTES
        add     hl,de
        ld      -33(ix),l
        ld      -34(ix),h

        ;; a polygon needs three points and the table has room for MAXPTS
        ld      a,4(ix)
        cp      #3
        jp      c,.fp_done
        cp      #MAXPTS+1
        jp      nc,.fp_done
        ;; an empty pattern draws nothing
        ld      a,9(ix)
        or      a
        jp      z,.fp_done

        ;; ---- edge table and bounding box ----
        xor     a
        ld      -5(ix),a                ; nedges = 0
        ld      -29(ix),a               ; i = 0
        ld      a,-3(ix)
        ld      -17(ix),a
        ld      a,-4(ix)
        ld      -18(ix),a               ; &pts[0]
        ld      l,-3(ix)
        ld      h,-4(ix)
        ld      de,#4
        add     hl,de
        ld      -19(ix),l
        ld      -20(ix),h               ; &pts[1]

        ;; the box starts at pts[0]
        ld      l,-3(ix)
        ld      h,-4(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -11(ix),e
        ld      -12(ix),d               ; xmin
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -7(ix),e
        ld      -8(ix),d                ; ymin
        ld      -9(ix),e
        ld      -10(ix),d               ; ymax

.fp_build:
        call    .load_edge_points
        call    .bbox_pi
        call    .edge_add

        ;; i++, step &pts[i], wrap &pts[j] back to pts[0] on the last edge
        ld      hl,#4
        ld      e,-17(ix)
        ld      d,-18(ix)
        add     hl,de
        ld      -17(ix),l
        ld      -18(ix),h
        ld      a,-29(ix)
        inc     a
        ld      -29(ix),a
        cp      4(ix)
        jr      nc,.fp_built
        inc     a
        cp      4(ix)
        jr      c,.fp_next_j
        ld      a,-3(ix)
        ld      -19(ix),a
        ld      a,-4(ix)
        ld      -20(ix),a
        jp      .fp_build
.fp_next_j:
        ld      hl,#4
        ld      e,-19(ix)
        ld      d,-20(ix)
        add     hl,de
        ld      -19(ix),l
        ld      -20(ix),h
        jp      .fp_build

.fp_built:
        ld      a,-5(ix)
        or      a
        jp      z,.fp_done              ; every edge was horizontal

        ;; ---- scanlines, ymin down to ymax ----
        ld      a,-7(ix)
        ld      -13(ix),a
        ld      a,-8(ix)
        ld      -14(ix),a               ; y = ymin
        xor     a
        ld      -15(ix),a               ; patt_idx = 0

.fp_row:
        xor     a
        ld      -16(ix),a
        ld      l,-13(ix)
        ld      h,-14(ix)
        ld      e,-9(ix)
        ld      d,-10(ix)
        or      a
        sbc     hl,de                   ; y - ymax
        jp      m,.fp_row_go            ; y < ymax
        ld      a,h
        or      l
        jp      nz,.fp_done             ; y > ymax: finished
        inc     a
        ld      -16(ix),a               ; y == ymax: the closing row
.fp_row_go:
        call    .row_crossings
        call    .row_spans

        ;; patt_idx = (patt_idx + 1) % fpatt_len
        ld      a,-15(ix)
        inc     a
        cp      9(ix)
        jr      c,.fp_row_patt
        xor     a
.fp_row_patt:
        ld      -15(ix),a

        ld      l,-13(ix)
        ld      h,-14(ix)
        inc     hl
        ld      -13(ix),l
        ld      -14(ix),h
        jp      .fp_row

.fp_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: n(1), c(1), m(1), fpatt(2), fpatt_len(1),
        ;; clip(2) = 8
        pop     hl
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        jp      (hl)

        ;; .load_edge_points
        ;; copy pts[i] and pts[j] into the pi/pj locals
        ;; clobbers: AF, DE, HL
.load_edge_points:
        ld      l,-17(ix)
        ld      h,-18(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -21(ix),e
        ld      -22(ix),d
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -23(ix),e
        ld      -24(ix),d
        ld      l,-19(ix)
        ld      h,-20(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -25(ix),e
        ld      -26(ix),d
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -27(ix),e
        ld      -28(ix),d
        ret

        ;; .bbox_pi
        ;; widen the bounding box to include pi
        ;; clobbers: AF, DE, HL
.bbox_pi:
        ld      e,-21(ix)
        ld      d,-22(ix)               ; pi.x
        ld      l,-11(ix)
        ld      h,-12(ix)
        or      a
        sbc     hl,de                   ; xmin - x
        jr      z,.bb_y
        jp      m,.bb_y
        ld      -11(ix),e
        ld      -12(ix),d
.bb_y:
        ld      e,-23(ix)
        ld      d,-24(ix)               ; pi.y
        ld      l,-7(ix)
        ld      h,-8(ix)
        or      a
        sbc     hl,de
        jr      z,.bb_ymax
        jp      m,.bb_ymax
        ld      -7(ix),e
        ld      -8(ix),d
.bb_ymax:
        ld      l,-9(ix)
        ld      h,-10(ix)
        or      a
        sbc     hl,de
        jp      p,.bb_done
        ld      -9(ix),e
        ld      -10(ix),d
.bb_done:
        ret

        ;; .edge_add
        ;; append one edge record for pi -> pj, normalized top to bottom.
        ;; A horizontal edge takes no slot: it contributes no crossing, and
        ;; the two edges that meet it carry the row.
        ;; clobbers: AF, BC, DE, HL, IY
.edge_add:
        ;; horizontal?
        ld      e,-23(ix)
        ld      d,-24(ix)
        ld      l,-27(ix)
        ld      h,-28(ix)
        or      a
        sbc     hl,de
        ret     z                       ; pi.y == pj.y

        ;; flipped = pi.y > pj.y, which is the sign of (pj.y - pi.y)
        ld      c,#0                    ; C = flipped
        jp      p,.ea_ordered
        ld      c,#1
        ;; swap pi and pj
        ld      e,-21(ix)
        ld      d,-22(ix)
        ld      l,-25(ix)
        ld      h,-26(ix)
        ld      -21(ix),l
        ld      -22(ix),h
        ld      -25(ix),e
        ld      -26(ix),d
        ld      e,-23(ix)
        ld      d,-24(ix)
        ld      l,-27(ix)
        ld      h,-28(ix)
        ld      -23(ix),l
        ld      -24(ix),h
        ld      -27(ix),e
        ld      -28(ix),d
.ea_ordered:
        ;; IY = the record being written
        ld      l,-1(ix)
        ld      h,-2(ix)
        push    hl
        pop     iy

        ;; y1 = pj.y
        ld      a,-27(ix)
        ld      E_Y1(iy),a
        ld      a,-28(ix)
        ld      E_Y1+1(iy),a
        ;; cx = pi.x, y0 = pi.y
        ld      a,-21(ix)
        ld      E_CX(iy),a
        ld      a,-22(ix)
        ld      E_CX+1(iy),a
        ld      a,-23(ix)
        ld      E_Y0(iy),a
        ld      a,-24(ix)
        ld      E_Y0+1(iy),a

        ;; dy = pj.y - pi.y, always positive here
        ld      l,-27(ix)
        ld      h,-28(ix)
        ld      e,-23(ix)
        ld      d,-24(ix)
        or      a
        sbc     hl,de
        ld      E_DY(iy),l
        ld      E_DY+1(iy),h

        ;; dx = |pj.x - pi.x|, sx = its sign
        ld      l,-25(ix)
        ld      h,-26(ix)
        ld      e,-21(ix)
        ld      d,-22(ix)
        or      a
        sbc     hl,de                   ; pj.x - pi.x
        xor     a
        bit     7,h
        jr      z,.ea_have_sx
        call    __gpx_neg_hl
        ld      a,#0x80
.ea_have_sx:
        ld      E_SX(iy),a
        ld      E_DX(iy),l
        ld      E_DX+1(iy),h

        ;; err = dx > dy ? bias(dx) : -bias(dy), where a flipped edge takes
        ;; (v-1)/2 so its walk lands on the same pixels as the outline's
        ld      e,E_DY(iy)
        ld      d,E_DY+1(iy)
        or      a
        sbc     hl,de                   ; dx - dy
        jr      z,.ea_ymajor
        jp      m,.ea_ymajor
        set     6,E_SX(iy)              ; x-major: every step advances x
        ld      l,E_DX(iy)
        ld      h,E_DX+1(iy)
        call    .bias
        jr      .ea_store_err
.ea_ymajor:
        ld      l,E_DY(iy)
        ld      h,E_DY+1(iy)
        call    .bias
        call    __gpx_neg_hl
.ea_store_err:
        ld      E_ERR(iy),l
        ld      E_ERR+1(iy),h

        ;; nedges++, advance the record cursor
        inc     -5(ix)
        ld      hl,#ERECSZ
        ld      e,-1(ix)
        ld      d,-2(ix)
        add     hl,de
        ld      -1(ix),l
        ld      -2(ix),h
        ret

        ;; .bias
        ;; param: HL = magnitude, C = 1 when the edge was flipped
        ;; return: HL = (C ? magnitude - 1 : magnitude) >> 1
        ;; clobbers: AF, HL
.bias:
        ld      a,c
        or      a
        jr      z,.bi_shift
        dec     hl
.bi_shift:
        srl     h
        rr      l
        ret


        ;; .row_crossings
        ;; collect this row's crossings into the crossing table
        ;; clobbers: AF, BC, DE, HL, IY, and the alternate set
.row_crossings:
        xor     a
        ld      -6(ix),a                ; nints = 0
        ld      b,-5(ix)                ; nonzero edge count
        ld      l,-31(ix)
        ld      h,-32(ix)
        push    hl
        pop     iy                      ; walk records directly
        ld      a,-33(ix)
        ld      -3(ix),a
        ld      a,-34(ix)
        ld      -4(ix),a                ; crossing cursor

.rc_loop:
        ;; the edge has not been reached yet?
        ld      l,-13(ix)
        ld      h,-14(ix)
        ld      e,E_Y0(iy)
        ld      d,E_Y0+1(iy)
        or      a
        sbc     hl,de
        jp      m,.rc_next              ; y < y0

        ;; past its end?  half-open, except on the closing row
        ld      l,-13(ix)
        ld      h,-14(ix)
        ld      e,E_Y1(iy)
        ld      d,E_Y1+1(iy)
        or      a
        sbc     hl,de                   ; y - y1
        jp      m,.rc_active
        ld      a,-16(ix)
        or      a
        jp      z,.rc_next              ; y >= y1
        ld      a,h
        or      l
        jp      nz,.rc_next             ; y > y1 even on the closing row

        ;; Keep a whole edge run in registers: HL = error, DE = current x,
        ;; BC = the minor delta. X-major runs need only one memory update
        ;; per row, however many pixels the edge crosses on that row.
        ;; Seed both endpoints with the first pixel, then replace just
        ;; the outer endpoint of an X-major run. The alternate register
        ;; set keeps the edge counter while HL/DE/BC walk the entire row.
.rc_active:
        exx
        ld      e,E_CX(iy)
        ld      d,E_CX+1(iy)
        ld      l,-3(ix)
        ld      h,-4(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      -3(ix),l
        ld      -4(ix),h
        inc     -6(ix)
        ld      l,E_ERR(iy)
        ld      h,E_ERR+1(iy)
        bit     6,E_SX(iy)
        jr      z,.rc_ymajor

        ld      c,E_DY(iy)
        ld      b,E_DY+1(iy)
        bit     7,E_SX(iy)
        jr      nz,.rc_xleft
.rc_xright:
        inc     de
        or      a
        sbc     hl,bc
        jp      p,.rc_xright
        jr      .rc_xerror
.rc_xleft:
        dec     de
        or      a
        sbc     hl,bc
        jp      p,.rc_xleft
.rc_xerror:
        ld      c,E_DX(iy)
        ld      b,E_DX+1(iy)
        add     hl,bc
        jr      .rc_save

.rc_ymajor:
        ;; Every Y-major row contains exactly one pixel. Only advance x
        ;; when the accumulated error becomes strictly positive.
        ld      c,E_DX(iy)
        ld      b,E_DX+1(iy)
        add     hl,bc
        ld      a,h
        or      l
        jr      z,.rc_save
        bit     7,h
        jr      nz,.rc_save
        ld      c,E_DY(iy)
        ld      b,E_DY+1(iy)
        or      a
        sbc     hl,bc
        bit     7,E_SX(iy)
        jr      nz,.rc_yleft
        inc     de
        jr      .rc_save
.rc_yleft:
        dec     de
.rc_save:
        ld      E_ERR(iy),l
        ld      E_ERR+1(iy),h
        ld      E_CX(iy),e
        ld      E_CX+1(iy),d
        bit     6,E_SX(iy)
        jr      z,.rc_walked
        ;; DE has reached the next row. The preceding pixel is this
        ;; row's outer endpoint: high x going right, low x going left.
        ld      l,-3(ix)
        ld      h,-4(ix)
        dec     hl
        dec     hl
        bit     7,E_SX(iy)
        jr      nz,.rc_left_end
        dec     de
        jr      .rc_endpoint
.rc_left_end:
        inc     de
        dec     hl
        dec     hl
.rc_endpoint:
        ld      (hl),e
        inc     hl
        ld      (hl),d
.rc_walked:
        exx

.rc_next:
        ld      de,#ERECSZ
        add     iy,de
        dec     b
        jp      nz,.rc_loop
        ret

        ;; .row_sort
        ;; Selection sort by low x. Keep the minimum in DE and its address
        ;; in IY, so each candidate is read once without stack-frame loads.
        ;; Equal crossings retain the original selection order. The caller
        ;; has already checked there are at least two crossings.
        ;; clobbers: AF, BC, DE, HL, IY
.row_sort:
        ld      a,-6(ix)
        dec     a
        ld      -29(ix),a
        ld      l,-33(ix)
        ld      h,-34(ix)               ; first unsorted record
.rs_outer:
        push    hl                      ; save the destination
        push    hl
        pop     iy                      ; address of the minimum
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; minimum low x
        inc     hl
        inc     hl
        inc     hl                      ; next crossing
        ld      b,-29(ix)
.rs_inner:
        ld      a,e
        sub     (hl)
        ld      c,a
        inc     hl
        ld      a,d
        sbc     a,(hl)                  ; minimum - candidate
        jp      m,.rs_inner_next
        or      c
        jr      z,.rs_inner_next
        ld      d,(hl)
        dec     hl
        ld      e,(hl)
        push    hl
        pop     iy
        inc     hl
.rs_inner_next:
        inc     hl
        inc     hl
        inc     hl
        djnz    .rs_inner

        pop     hl                      ; destination
        push    iy
        pop     de                      ; minimum's address
        push    hl
        or      a
        sbc     hl,de
        pop     hl
        jr      z,.rs_no_swap
        ld      b,#4
.rs_swap_byte:
        ld      a,(de)
        ld      c,a
        ld      a,(hl)
        ld      (de),a
        ld      a,c
        ld      (hl),a
        inc     hl
        inc     de
        djnz    .rs_swap_byte
        jr      .rs_outer_next
.rs_no_swap:
        ld      de,#4
        add     hl,de
.rs_outer_next:
        dec     -29(ix)
        jr      nz,.rs_outer
        ret

        ;; .row_spans
        ;; draw this row's spans, pairing crossings even-odd
        ;; clobbers: AF, BC, DE, HL, IY
.row_spans:
        ld      a,-6(ix)
        cp      #2
        ret     c
        ld      e,-15(ix)
        ld      d,#0
        ld      l,7(ix)
        ld      h,8(ix)
        add     hl,de
        ld      a,(hl)                  ; fpatt[patt_idx]
        or      a
        ret     z                       ; no spans can ink an empty row
        cp      #0xFF
        jr      z,.rp_pattern_ready
        ld      b,#8
.rp_reverse:
        rlca
        rr      c
        djnz    .rp_reverse
        ld      a,c
.rp_pattern_ready:
        ld      -30(ix),a               ; reversed byte, before span rotation
        call    .row_sort               ; empty pattern rows need no sorting

        ld      a,-33(ix)
        ld      -21(ix),a
        ld      a,-34(ix)
        ld      -22(ix),a               ; cursor at the first crossing
        ;; Seed the previous end just left of the leftmost crossing. A
        ;; sentinel like 0x8000 would overflow the signed compares below.
        ld      l,-21(ix)
        ld      h,-22(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        dec     de
        ld      -25(ix),e
        ld      -26(ix),d
        ld      a,-6(ix)
        srl     a
        ld      -29(ix),a               ; pairs to draw

.rp_pair:
        ld      a,-29(ix)
        or      a
        ret     z

        ;; x0 = this crossing's low x
        ld      l,-21(ix)
        ld      h,-22(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -23(ix),e
        ld      -24(ix),d
        ;; x1 = the next crossing's high x
        ld      l,-21(ix)
        ld      h,-22(ix)
        ld      bc,#6
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -27(ix),e
        ld      -28(ix),d

        ;; two spans can meet on a shared vertex pixel: leave it to the
        ;; first, so BM_XOR does not cancel it
        ld      l,-23(ix)
        ld      h,-24(ix)
        ld      e,-25(ix)
        ld      d,-26(ix)
        or      a
        sbc     hl,de                   ; x0 - prev_end
        jp      m,.rp_trim
        ld      a,h
        or      l
        jr      nz,.rp_check
.rp_trim:
        ex      de,hl                   ; previous end
        inc     hl
        ld      -23(ix),l
        ld      -24(ix),h

.rp_check:
        ;; anything left of the span?
        ld      l,-23(ix)
        ld      h,-24(ix)
        ld      e,-27(ix)
        ld      d,-28(ix)
        or      a
        sbc     hl,de                   ; x0 - x1
        jr      z,.rp_draw
        jp      p,.rp_next              ; x0 > x1: nothing to draw
.rp_draw:
        ld      a,-27(ix)
        ld      -25(ix),a
        ld      a,-28(ix)
        ld      -26(ix),a               ; prev_end = x1

        ;; Draw one span with this row's pattern byte, turned from the
        ;; fill's MSB-first-from-the-box convention into the LSB-first one
        ;; a line consumes
        ;; Clobbers AF, BC, DE, HL and IY; IX remains the polygon frame.
.span_line:
        ld      a,-30(ix)
        cp      #0xFF
        jr      z,.sl_draw
        ld      a,-23(ix)
        sub     -11(ix)                 ; (x0 - xmin) modulo eight
        and     #0x07
        ld      b,a
        ld      a,-30(ix)
        jr      z,.sl_draw
.sl_rotate:
        rrca
        djnz    .sl_rotate

.sl_draw:
        ;; The private horizontal entry accepts the same stack arguments
        ;; as gpx_draw_line, preserves IX, and does not use HL = gpx.
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl                      ; clip
        push    af
        inc     sp                      ; lpatt
        ld      l,5(ix)
        ld      h,6(ix)
        push    hl                      ; c, m
        ld      l,-13(ix)
        ld      h,-14(ix)
        push    hl                      ; y1
        ld      l,-27(ix)
        ld      h,-28(ix)
        push    hl                      ; x1
        ld      l,-13(ix)
        ld      h,-14(ix)
        push    hl                      ; y0
        ld      e,-23(ix)
        ld      d,-24(ix)               ; DE = x0
        call    __gpx_hline

.rp_next:
        ld      hl,#8
        ld      e,-21(ix)
        ld      d,-22(ix)
        add     hl,de
        ld      -21(ix),l
        ld      -22(ix),h
        dec     -29(ix)
        jp      .rp_pair
