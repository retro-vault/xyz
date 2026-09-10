        ;; gpx_fill_rectangle.s
        ;;
        ;; Rectangle fill renderer:
        ;;  - normalizes rectangle coordinates
        ;;  - pattern is applied per row, MSB-first from x0
        ;;  - masks, byte count, plot selectors and the pattern rotation are
        ;;    all constant down the rectangle, so they are computed once and
        ;;    each row is a single __gpx_span_row call with the row pointer
        ;;    stepped by __vid_nextrow
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_fill_rectangle
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_fill_rectangle
        .globl  __gpx_span_row
        .globl  __gpx_span_setup
        .globl  __rect_unpack_norm
        .globl  __clip_seg
        .globl  __rect_screen
        .globl  __vid_rowaddr
        .globl  __vid_nextrow

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_fill_rectangle
        ;; Fill a rectangle with a repeating pattern, corners inclusive.
        ;; Clipping is resolved once up front, then rows are drawn by byte
        ;; spans rather than pixels. Row n takes fpatt[n % fpatt_len] and
        ;; the bits run MSB-first from the rectangle's left edge, both
        ;; measured on the unclipped rectangle so clipping never shifts
        ;; the pattern.
        ;;
        ;; Signature:
        ;;   void gpx_fill_rectangle(gpx_t *gpx, rect_t *r,
        ;;                           color c, bmode m,
        ;;                           uint8_t *fpatt, uint8_t fpatt_len,
        ;;                           const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = r
        ;;   stack: c, m, fpatt, fpatt_len, clip
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   __gpx_span_setup
        ;;   __gpx_span_row
        ;;   __vid_rowaddr, __vid_nextrow
        ;;   __rect_cmp16s_lt
_gpx_fill_rectangle::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (21 bytes)
        ;; -1..-2   x0
        ;; -3..-4   x1
        ;; -5..-6   y0
        ;; -7..-8   y1
        ;; -9..-10  row pointer into pixel VRAM
        ;; -11..-12 y0 original / ycur
        ;; -13      fpatt idx
        ;; -14..-15 fpatt ptr
        ;; -16      fpatt len
        ;; -17..-21 span descriptor for __gpx_span_row:
        ;;          -21 mask_first, -20 mask_last, -19 count,
        ;;          -18 sel_or,     -17 sel_xor
        ;; -22      x phase shift (x0 original & 7)
        ld      hl,#-22
        add     hl,sp
        ld      sp,hl

        ;; if (r == NULL) return
        ld      a,d
        or      e
        jp      z,.fr_done

        ;; if (fpatt_len == 0) return
        ld      a,8(ix)
        or      a
        jp      z,.fr_done

        ;; save fpatt ptr + len
        ld      a,6(ix)
        ld      -14(ix),a
        ld      a,7(ix)
        ld      -15(ix),a
        ld      a,8(ix)
        ld      -16(ix),a

        ;; unpack + normalize rect into locals
        call    __rect_unpack_norm

        ;; preserve original y0 for pattern-phase alignment
        ld      a,-5(ix)
        ld      -11(ix),a
        ld      a,-6(ix)
        ld      -12(ix),a
        ;; The pattern is anchored to the rectangle's own x0, and every
        ;; destination byte is 8-aligned, so one rotation by (x0 & 7) puts
        ;; the pattern on the byte grid for the whole rectangle.
        ld      a,-1(ix)
        and     #0x07
        ld      -22(ix),a

        ;; Clamp to the screen. This is the same 1-D clamp the clip rect
        ;; needs, so it runs through the shared helper against __rect_screen
        ;; rather than being open-coded here.
        ld      iy,#__rect_screen
        ld      l,-1(ix)
        ld      h,-2(ix)
        ld      e,-3(ix)
        ld      d,-4(ix)
        call    __clip_seg
        jp      c,.fr_done
        ld      -1(ix),l
        ld      -2(ix),h
        ld      -3(ix),e
        ld      -4(ix),d

        ld      iy,#__rect_screen+2
        ld      l,-5(ix)
        ld      h,-6(ix)
        ld      e,-7(ix)
        ld      d,-8(ix)
        call    __clip_seg
        jp      c,.fr_done
        ld      -5(ix),l
        ld      -6(ix),h
        ld      -7(ix),e
        ld      -8(ix),d

        ;; optional clip visible range once:
        ;;   [x0..x1] ∩ [clip->x0..clip->x1]
        ;;   [y0..y1] ∩ [clip->y0..clip->y1]
        ld      a,9(ix)
        or      10(ix)
        jr      z,.fr_phase_setup

        ;; Clip [x0..x1] and [y0..y1] against the clip rect once, via the
        ;; shared __clip_seg helper (reject on either axis => nothing visible).
        ;; X axis: IY = &clip->x0 (clip+0); x1 at IY+4.
        ld      c,9(ix)
        ld      b,10(ix)                ; BC = clip ptr
        ld      iy,#0
        add     iy,bc
        ld      l,-1(ix)
        ld      h,-2(ix)                ; HL = x0
        ld      e,-3(ix)
        ld      d,-4(ix)                ; DE = x1
        call    __clip_seg
        jp      c,.fr_done
        ld      -1(ix),l
        ld      -2(ix),h                ; x0 = clamped lo
        ld      -3(ix),e
        ld      -4(ix),d                ; x1 = clamped hi

        ;; Y axis: IY = &clip->y0 (clip+2); y1 at IY+4 (clip+6).
        ld      c,9(ix)
        ld      b,10(ix)                ; reload BC = clip ptr
        ld      iy,#2
        add     iy,bc
        ld      l,-5(ix)
        ld      h,-6(ix)                ; HL = y0
        ld      e,-7(ix)
        ld      d,-8(ix)                ; DE = y1
        call    __clip_seg
        jp      c,.fr_done
        ld      -5(ix),l
        ld      -6(ix),h                ; y0 = clamped lo
        ld      -7(ix),e
        ld      -8(ix),d                ; y1 = clamped hi

.fr_phase_setup:
        ;; IY = &descriptor (ix - 21)
        push    ix
        pop     iy
        ld      de,#-21
        add     iy,de

        ld      b,-1(ix)                ; x0 (clamped to the screen)
        ld      c,-3(ix)                ; x1
        ld      d,4(ix)                 ; color
        ld      e,5(ix)                 ; mode
        call    __gpx_span_setup        ; A = byte_lo
        ld      c,a

        ;; row pointer for the first visible row
        ld      b,-5(ix)                ; y0 low
        call    __vid_rowaddr
        ld      a,c                     ; byte_lo
        add     a,l
        ld      l,a
        push    hl                      ; keep first row across phase arithmetic

.fr_idx_setup:
        ;; idx = (y0_clipped - y0_original) % fpatt_len
        ld      l,-5(ix)
        ld      h,-6(ix)
        ld      e,-11(ix)
        ld      d,-12(ix)
        xor     a
        sbc     hl,de
        ;; A fixed 16-bit remainder bounds the work even when the original
        ;; y is -32768. The frequent already-in-range case still returns
        ;; immediately, before entering the bit loop.
        ld      c,-16(ix)
        ld      a,h
        or      a
        jr      nz,.fr_idx_div
        ld      a,l
        cp      c
        jr      c,.fr_idx_done
.fr_idx_div:
        ld      b,#16
        xor     a
.fr_idx_mod:
        add     hl,hl
        rla                             ; shift the next dividend bit into A
        jr      c,.fr_idx_sub           ; ninth remainder bit: subtract always
        cp      c
        jr      c,.fr_idx_next
.fr_idx_sub:
        sub     c
.fr_idx_next:
        djnz    .fr_idx_mod
.fr_idx_done:
        ld      -13(ix),a

        ;; row count = y1 - y0 + 1 (both already clamped to the screen)
        ld      a,-7(ix)
        sub     -5(ix)
        inc     a
        ld      -11(ix),a
        pop     hl                      ; row pointer remains live through the loop

.fr_row_loop:
        push    hl
        ;; pattern for this row, rotated onto the byte grid
        ld      l,-14(ix)
        ld      h,-15(ix)
        ld      e,-13(ix)
        ld      d,#0x00
        add     hl,de
        ld      a,(hl)
        ld      b,-22(ix)
        inc     b
        dec     b
        jr      z,.fr_patt_ready
.fr_patt_rot:
        rrca
        djnz    .fr_patt_rot
.fr_patt_ready:
        pop     hl
        call    __gpx_span_row          ; preserves HL
        dec     -11(ix)
        jr      z,.fr_done
        call    __vid_nextrow

        ;; idx = (idx + 1) % fpatt_len
        ld      a,-13(ix)
        inc     a
        cp      -16(ix)
        jr      c,.fr_store_idx
        xor     a
.fr_store_idx:
        ld      -13(ix),a

        jr      .fr_row_loop

.fr_done:
        ld      sp,ix
        pop     ix

        ;; callee cleanup: c(1), m(1), fpatt(2), fpatt_len(1), clip(2) = 7
        pop     de
        ld      hl,#7
        add     hl,sp
        ld      sp,hl
        push    de
        ret
