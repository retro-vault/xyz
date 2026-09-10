        ;; _rect_helpers.s
        ;;
        ;; Minimal signed 16-bit compare helper.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _rect_helpers
        .optsdcc -mz80 sdcccall(1)

        .globl  __rect_cmp16s_lt
        .globl  __rect_unpack_norm
        .globl  __clip_seg
        .globl  __rect_screen
        .globl  __ret_clean11

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __ret_clean11
        ;;
        ;; Shared callee-cleanup tail for the line-family entry points
        ;; (draw_line/hline/vline/bresenham all clean 11 stack bytes).
        ;; Enter via jp with the return value in A and the caller frame
        ;; already unwound to [ret][args]. Clobbers DE/HL.
        ;; ------------------------------------------------------------
__ret_clean11:
        pop     de                      ; return address
        ld      hl,#11
        add     hl,sp
        ld      sp,hl
        push    de
        ret

        ;; ------------------------------------------------------------
        ;; __rect_screen
        ;;
        ;; The whole display as a rect_t, so a screen clamp is just another
        ;; __clip_seg pass instead of open-coded 16-bit compares in every
        ;; primitive.
        ;; ------------------------------------------------------------
__rect_screen::
        .dw     0
        .dw     0
        .dw     255
        .dw     191

        ;; ------------------------------------------------------------
        ;; __clip_seg
        ;;
        ;; 1-D segment clip against one clip-rect axis (primary axis):
        ;;   reject if (clip_hi < seg_lo) || (seg_hi < clip_lo)
        ;;   else clamp seg_lo = max(seg_lo, clip_lo),
        ;;             seg_hi = min(seg_hi, clip_hi)
        ;;   reject if the clamp inverted the span (swapped clip rect)
        ;;
        ;; Pure register I/O so any caller's local layout works:
        ;;   IN:  HL = seg_lo, DE = seg_hi
        ;;        IY = &clip primary-axis lo field (hi field is at IY+4,
        ;;             which holds for both x0/x1 and y0/y1 in rect_t).
        ;;   OUT: HL = clamped seg_lo, DE = clamped seg_hi
        ;;        CARRY set  => reject (segment outside clip on this axis)
        ;;        CARRY clear => keep (HL/DE updated)
        ;;
        ;; Relies on __rect_cmp16s_lt preserving BC/DE/HL (only A changes).
        ;; Preserves IX and IY; uses A, BC, DE, HL and one stack slot.
        ;; ------------------------------------------------------------
__clip_seg:
        ;; Clamp both ends, then reject if the clamp inverted the span. That
        ;; single test subsumes the two early rejects this used to do: if
        ;; seg_hi < clip_lo or clip_hi < seg_lo, the clamped span is inverted
        ;; by construction. Five signed compares become three.
        ld      b,h
        ld      c,l                     ; BC = seg_lo
        push    de                      ; stack: [seg_hi]

        ;; seg_lo = max(seg_lo, clip_lo)
        ld      e,0(iy)
        ld      d,1(iy)                 ; DE = clip_lo, HL = seg_lo
        call    __rect_cmp16s_lt
        jr      nc,.cs_lo_ok
        ld      c,e
        ld      b,d                     ; seg_lo = clip_lo
.cs_lo_ok:
        pop     de                      ; DE = seg_hi

        ;; seg_hi = min(seg_hi, clip_hi)
        ld      l,4(iy)
        ld      h,5(iy)                 ; HL = clip_hi
        call    __rect_cmp16s_lt        ; clip_hi < seg_hi ?
        jr      nc,.cs_hi_ok
        ld      e,l
        ld      d,h                     ; seg_hi = clip_hi
.cs_hi_ok:

        ;; inverted span => nothing visible on this axis
        ld      l,e
        ld      h,d                     ; HL = seg_hi
        ld      e,c
        ld      d,b                     ; DE = seg_lo
        call    __rect_cmp16s_lt        ; seg_hi < seg_lo ?
        ex      de,hl                   ; HL = seg_lo, DE = seg_hi
        ret                             ; comparison already returned the carry


        ;; ------------------------------------------------------------
        ;; __rect_cmp16s_lt
        ;; Compare signed HL < DE, preserving BC/DE/HL. Carry is sign XOR
        ;; overflow of the 16-bit subtraction. RLA copies sign to carry
        ;; while preserving P/V, so only signed overflow needs an inversion.
        ;; A is scratch; callers needing a 0/1 value synthesize it from carry.
__rect_cmp16s_lt:
        ld      a,l
        sub     e
        ld      a,h
        sbc     a,d
        rla
        ret     po
        ccf
        ret

        ;; ------------------------------------------------------------
        ;; __rect_unpack_norm
        ;;
        ;; Unpack rect_t from DE directly into the caller frame at IX
        ;; using caller-local layout:
        ;;   [-1..-2] x0, [-3..-4] x1, [-5..-6] y0, [-7..-8] y1
        ;; and normalize endpoints so x0<=x1 and y0<=y1.
        ;;
        ;;   DE = const rect_t *src (x0,y0,x1,y1 in struct order)
        ;;   IX = caller frame base (preserved)
        ;; ------------------------------------------------------------
__rect_unpack_norm:
        ;; x0
        ld      a,(de)
        ld      -1(ix),a
        inc     de
        ld      a,(de)
        ld      -2(ix),a
        inc     de

        ;; y0 -> [-5..-6]
        ld      a,(de)
        ld      -5(ix),a
        inc     de
        ld      a,(de)
        ld      -6(ix),a
        inc     de

        ;; x1 -> [-3..-4]
        ld      a,(de)
        ld      -3(ix),a
        inc     de
        ld      a,(de)
        ld      -4(ix),a
        inc     de

        ;; y1 -> [-7..-8]
        ld      a,(de)
        ld      -7(ix),a
        inc     de
        ld      a,(de)
        ld      -8(ix),a

        ;; if (x1 < x0) swap
        ld      l,-3(ix)
        ld      h,-4(ix)
        ld      e,-1(ix)
        ld      d,-2(ix)
        call    __rect_cmp16s_lt
        jr      nc,.ru_x_ok

        ld      -1(ix),l
        ld      -2(ix),h
        ld      -3(ix),e
        ld      -4(ix),d

.ru_x_ok:
        ;; if (y1 < y0) swap
        ld      l,-7(ix)
        ld      h,-8(ix)
        ld      e,-5(ix)
        ld      d,-6(ix)
        call    __rect_cmp16s_lt
        jr      nc,.ru_done

        ld      -5(ix),l
        ld      -6(ix),h
        ld      -7(ix),e
        ld      -8(ix),d

.ru_done:
        ret
