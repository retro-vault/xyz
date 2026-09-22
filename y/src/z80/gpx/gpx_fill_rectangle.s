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
        .globl  __gpx_span_row_copy
        .globl  __gpx_span_setup
        .globl  __rect_unpack_norm
        .globl  __clip_seg
        .globl  __vid_rowaddr
        .globl  __vid_nextrow

        .globl  __frame_ix

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
        ;;   AF, BC, DE, HL. Preserves IX and IY.
        ;;
        ;; References:
        ;;   __gpx_span_setup
        ;;   __gpx_span_row
        ;;   __vid_rowaddr, __vid_nextrow
        ;;   __rect_unpack_norm, __clip_seg
_gpx_fill_rectangle::
        push    iy
        call    __frame_ix

        ;; locals (17 bytes):
        ;; -8..-1   normalized rect_t: x0, y0, x1, y1 (little-endian)
        ;; -10..-9  original y0 (high, low); -9 becomes remaining row count
        ;; -11      fill pattern index (pointer and length stay in arguments)
        ;; -16..-12 span descriptor: first/last masks, count, OR/XOR selectors
        ;; -17      x phase shift (original x0 & 7)
        ld      hl,#-17
        add     hl,sp
        ld      sp,hl

        ;; if (r == NULL) return
        ld      a,d
        or      e
        jp      z,.fr_done

        ;; if (fpatt_len == 0) return
        ld      a,10(ix)
        or      a
        jp      z,.fr_done

        ;; unpack + normalize rect into locals
        call    __rect_unpack_norm

        ;; preserve original y0 for pattern-phase alignment
        ld      a,-6(ix)
        ld      -9(ix),a
        ld      a,-5(ix)
        ld      -10(ix),a
        ;; The pattern is anchored to the rectangle's own x0, and every
        ;; destination byte is 8-aligned, so one rotation by (x0 & 7) puts
        ;; the pattern on the byte grid for the whole rectangle.
        ld      a,-8(ix)
        and     #0x07
        ld      -17(ix),a

        ;; Each axis stays in registers through screen and optional clipping.
        ;; Only visible low bytes are needed after it is accepted.
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      c,11(ix)
        ld      b,12(ix)
        ld      a,#255
        call    .fr_clip_axis
        jp      c,.fr_done
        ld      -8(ix),l
        ld      -4(ix),e

        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      c,11(ix)
        ld      b,12(ix)
        ld      a,b
        or      c
        jr      z,.fr_y_clip
        inc     bc
        inc     bc
.fr_y_clip:
        ld      a,#191
        call    .fr_clip_axis
        jp      c,.fr_done
        ld      -6(ix),l
        ld      -2(ix),e

.fr_phase_setup:
        ;; IY = &descriptor (ix - 16)
        push    ix
        pop     iy
        ld      de,#-16
        add     iy,de

        ld      b,-8(ix)                ; x0 (clamped to the screen)
        ld      c,-4(ix)                ; x1
        ld      d,6(ix)                 ; color
        ld      e,7(ix)                 ; mode
        call    __gpx_span_setup        ; A = byte_lo
        ld      c,a

        ;; row pointer for the first visible row
        ld      b,-6(ix)                ; y0 low
        call    __vid_rowaddr
        ld      a,c                     ; byte_lo
        add     a,l
        ld      l,a
        push    hl                      ; keep first row across phase arithmetic

.fr_idx_setup:
        ;; idx = (y0_clipped - y0_original) % fpatt_len
        ld      l,-6(ix)
        ld      h,#0
        ld      e,-9(ix)
        ld      d,-10(ix)
        xor     a
        sbc     hl,de
        ;; A fixed 16-bit remainder bounds the work even when the original
        ;; y is -32768. The frequent already-in-range case still returns
        ;; immediately, before entering the bit loop.
        ld      c,10(ix)
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
        ld      -11(ix),a

        ;; row count = y1 - y0 + 1 (both already clamped to the screen)
        ld      a,-2(ix)
        sub     -6(ix)
        inc     a
        ld      -9(ix),a
        pop     hl                      ; row pointer remains live through the loop

.fr_row_loop:
        push    hl
        ;; pattern for this row, rotated onto the byte grid
        ld      l,8(ix)
        ld      h,9(ix)
        ld      e,-11(ix)
        ld      d,#0x00
        add     hl,de
        ld      a,(hl)
        ld      b,-17(ix)
        inc     b
        dec     b
        jr      z,.fr_patt_ready
.fr_patt_rot:
        rrca
        djnz    .fr_patt_rot
.fr_patt_ready:
        pop     hl
        ld      b,7(ix)
        inc     b
        dec     b                       ; test mode without disturbing pattern
        jr      nz,.fr_row_stipple

        ;; Readable framebuffer: fold CO_BACK by complementing the desired
        ;; pattern, then replace the covered bits in one byte-span pass.
        ld      b,6(ix)
        bit     0,b
        jr      nz,.fr_copy_ready
        cpl
.fr_copy_ready:
        call    __gpx_span_row_copy
        jr      .fr_row_done
.fr_row_stipple:
        call    __gpx_span_row          ; preserves HL
.fr_row_done:
        dec     -9(ix)
        jr      z,.fr_done
        call    __vid_nextrow

        ;; idx = (idx + 1) % fpatt_len
        ld      a,-11(ix)
        inc     a
        cp      10(ix)
        jr      c,.fr_store_idx
        xor     a
.fr_store_idx:
        ld      -11(ix),a

        jr      .fr_row_loop

.fr_done:
        ld      sp,ix
        pop     ix
        pop     iy

        ;; callee cleanup: c(1), m(1), fpatt(2), fpatt_len(1), clip(2) = 7
        pop     de
        ld      hl,#7
        add     hl,sp
        ld      sp,hl
        push    de
        ret

        ;; HL=lo, DE=hi, A=screen maximum, BC=optional clip axis.
        ;; Return on-screen endpoints in HL/DE; carry rejects an empty span.
.fr_clip_axis:
        push    bc
        ld      c,a
        ld      a,h
        or      a
        jr      z,.fr_axis_lo
        jp      p,.fr_axis_empty
        ld      hl,#0
.fr_axis_lo:
        ld      a,c
        cp      l
        jr      c,.fr_axis_empty
        ld      a,d
        or      a
        jr      z,.fr_axis_hi
        jp      m,.fr_axis_empty
        ld      e,c
.fr_axis_hi:
        ld      a,c
        cp      e
        jr      nc,.fr_axis_keep
        ld      e,c
.fr_axis_keep:
        ld      h,#0
        ld      d,h
        ld      a,e
        cp      l
        jr      c,.fr_axis_empty
        pop     bc
        ld      a,b
        or      c
        ret     z
        push    bc
        pop     iy
        jp      __clip_seg
.fr_axis_empty:
        pop     bc
        scf
        ret
