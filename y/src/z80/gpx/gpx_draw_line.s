        ;; gpx_draw_line.s
        ;;
        ;; ZX Spectrum line dispatcher.
        ;;
        ;; Frame-less: args are peeked SP-relative (no IX), so the fast
        ;; hline/vline/bresenham targets receive the caller frame untouched
        ;; via tail jumps.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_draw_line
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_line
        .globl  __gpx_plot_raw

        .globl  __gpx_hline
        .globl  __gpx_bresenham_line
        .globl  __rect_cmp16s_lt
        .globl  __ret_clean11

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_line
        ;; Dispatch strategy:
        ;;   if x0==x1 and y0==y1 -> draw pixel
        ;;   else if x0==x1       -> vertical line routine (y0<=y1)
        ;;   else if y0==y1       -> horizontal line routine (x0<=x1)
        ;;   else                 -> Bresenham routine
        ;;
        ;; Signature:
        ;;   uint8_t gpx_draw_line(gpx_t *gpx,
        ;;                         coord x0, coord y0, coord x1, coord y1,
        ;;                         color c, bmode m, uint8_t lpatt,
        ;;                         const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx (unused), DE = x0
        ;;   stack: [ret][y0 @2][x1 @4][y1 @6][c @8][m @9][lpatt @10][clip @11]
        ;;
        ;; Return:
        ;;   A = the pattern rotated by the number of pixels drawn, so
        ;;       chained segments keep their phase
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY, and the alternate set
        ;;
        ;; References:
        ;;   __gpx_hline
        ;;   __gpx_bresenham_line
        ;;   __gpx_plot_raw
_gpx_draw_line::
        ld      hl,#2
        add     hl,sp                   ; HL -> y0
        ld      c,(hl)
        inc     hl
        ld      b,(hl)                  ; BC = y0
        inc     hl                      ; HL -> x1

        ;; x0 == x1 ?
        ld      a,(hl)
        cp      e
        jr      nz,.gl_not_v
        inc     hl
        ld      a,(hl)
        cp      d
        jr      nz,.gl_not_v

        ;; y0 == y1 ?
        inc     hl                      ; HL -> y1
        ld      a,(hl)
        cp      c
        jr      nz,.gl_bres             ; vertical: bres has a fast path
        inc     hl
        ld      a,(hl)
        cp      b
        jr      nz,.gl_bres

        ;; single pixel: honor lpatt bit0
        ld      hl,#10
        add     hl,sp                   ; HL -> lpatt
        ld      a,(hl)
        and     #0x01
        jr      z,.gl_single_done

        ;; The point already lives in registers. Feed the raw pixel entry
        ;; directly, keeping y while BC is loaded with the clip pointer.
        push    bc
        inc     hl                      ; HL -> clip
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        ld      hl,#10                  ; c,m after the saved y word
        add     hl,sp
        ld      a,(hl)                  ; color bit into carry
        rrca
        inc     hl
        ld      a,(hl)                  ; mode bit becomes bit 1
        rla
        pop     hl                      ; y; DE still holds x
        call    __gpx_plot_raw

.gl_single_done:
        ;; degenerate line: return lpatt unrotated
        ld      hl,#10
        add     hl,sp
        ld      a,(hl)
        jp      __ret_clean11           ; callee cleanup: 11 bytes

.gl_not_v:
        ;; y0 == y1 ?
        ld      hl,#6
        add     hl,sp                   ; HL -> y1
        ld      a,(hl)
        cp      c
        jr      nz,.gl_bres
        inc     hl
        ld      a,(hl)
        cp      b
        jr      nz,.gl_bres

        ;; fast hline assumes x0 <= x1; reversed goes through Bresenham
        ld      hl,#4
        add     hl,sp
        ld      a,(hl)
        inc     hl
        ld      h,(hl)
        ld      l,a                     ; HL = x1
        call    __rect_cmp16s_lt        ; x1 < x0 ?
        jr      c,.gl_bres
        jp      __gpx_hline

.gl_bres:
        jp      __gpx_bresenham_line
