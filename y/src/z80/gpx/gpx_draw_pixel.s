        ;; gpx_draw_pixel.s
        ;;
        ;; ZX Spectrum pixel primitive.
        ;;
        ;; The drawing logic lives in a register-fed internal core
        ;; (__gpx_plot_raw) with no IX frame, so hot callers (Bresenham) can
        ;; plot a pixel without per-pixel frame setup/teardown. The public
        ;; gpx_draw_pixel is a thin wrapper that marshals its stack args into
        ;; the core's register interface. Clip/bounds/plot behavior is byte
        ;; identical to the previous implementation (the oracle clips per pixel,
        ;; so this stays on the same path).
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_draw_pixel
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_pixel
        .globl  __gpx_plot_raw
        .globl  __vid_rowaddr

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; void gpx_draw_pixel(
        ;;   gpx_t *gpx,        HL (ignored)
        ;;   coord x,           DE
        ;;   coord y,           stack
        ;;   color c,           stack (1 byte)
        ;;   bmode m,           stack (1 byte)
        ;;   const rect_t *clip stack (2 bytes)
        ;;
        ;; Callee cleans 6 bytes: args are popped straight into the
        ;; __gpx_plot_raw register interface (no IX frame, no epilogue).
        ;; Return address is parked in BC' while the stack is drained,
        ;; then pushed back so plot_raw's ret goes to the caller.
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, and the alternate set
        ;;
        ;; References:
        ;;   __gpx_plot_raw
        ;; ------------------------------------------------------------
_gpx_draw_pixel::
        exx
        pop     bc                      ; BC' = return address
        exx
        pop     hl                      ; HL = y
        pop     af                      ; A = m, F = c (carry = color bit0)
        rla                             ; A = (m<<1) | color
        and     #0x03                   ; packed flags
        pop     bc                      ; BC = clip
        exx
        push    bc                      ; return address back on stack
        exx
        ;; fall through into __gpx_plot_raw (its ret returns to caller)

        ;; ------------------------------------------------------------
        ;; __gpx_plot_raw  (internal, register interface, no IX frame)
        ;;   DE = x (signed 16-bit)
        ;;   HL = y (signed 16-bit)
        ;;   BC = clip rect pointer (0 => no clip)
        ;;   A  = packed flags: bit0 = color (CO_FORE), bit1 = mode (BM_XOR)
        ;; Clobbers A, BC, DE, HL (and F). Preserves IX/IY.
        ;; ------------------------------------------------------------
__gpx_plot_raw:
        push    af                      ; preserve packed color/mode (survives rowaddr)

        ;; x in [0,255]?  (hi byte must be 0)
        ld      a,d
        or      a
        jp      nz,.pr_reject
        ;; y in [0,191]?
        ld      a,h
        or      a
        jp      nz,.pr_reject
        ld      a,l
        cp      #192
        jr      nc,.pr_reject

        ;; optional clip (BC = rect ptr)
        ld      a,b
        or      c
        jr      z,.pr_plot_nl           ; no clip: y lo in L, x lo in E

        ;; stash y lo in D (free, hi was 0), walk clip via HL
        ld      d,l                     ; D = y
        ld      h,b
        ld      l,c                     ; HL = clip ptr

        ;; if (x < clip->x0) reject
        ld      a,(hl)                  ; x0 lo
        inc     hl
        ld      b,(hl)                  ; x0 hi
        inc     hl                      ; HL -> &y0
        bit     7,b
        jr      nz,.pr_cy0              ; x0 < 0 => pass
        ld      c,a                     ; save x0 lo
        ld      a,b
        or      a
        jr      nz,.pr_reject           ; x0 >= 256 => x < x0 => reject
        ld      a,e                     ; x
        cp      c
        jr      c,.pr_reject            ; x < x0

.pr_cy0:
        ;; if (y < clip->y0) reject
        ld      a,(hl)                  ; y0 lo
        inc     hl
        ld      b,(hl)                  ; y0 hi
        inc     hl                      ; HL -> &x1
        bit     7,b
        jr      nz,.pr_cx1
        ld      c,a
        ld      a,b
        or      a
        jr      nz,.pr_reject
        ld      a,d                     ; y
        cp      c
        jr      c,.pr_reject

.pr_cx1:
        ;; if (x > clip->x1) reject  (== clip->x1 < x)
        ld      c,(hl)                  ; x1 lo
        inc     hl
        ld      b,(hl)                  ; x1 hi
        inc     hl                      ; HL -> &y1
        bit     7,b
        jr      nz,.pr_reject           ; x1 < 0 => reject
        ld      a,b
        or      a
        jr      nz,.pr_cy1              ; x1 >= 256 => pass
        ld      a,c                     ; x1 lo
        cp      e                       ; x1 < x ?
        jr      c,.pr_reject

.pr_cy1:
        ;; if (y > clip->y1) reject
        ld      c,(hl)                  ; y1 lo
        inc     hl
        ld      b,(hl)                  ; y1 hi
        bit     7,b
        jr      nz,.pr_reject           ; y1 < 0 => reject
        ld      a,b
        or      a
        jr      nz,.pr_plot             ; y1 >= 256 => pass
        ld      a,c                     ; y1 lo
        cp      d                       ; y1 < y ?
        jr      c,.pr_reject

.pr_plot:
        ;; y in D, x in E
        ld      b,d
        jr      .pr_plot_common

.pr_plot_nl:
        ;; y in L, x in E
        ld      b,l

.pr_plot_common:
        ;; mask = 0x80 >> (x & 7), from a table while HL is still free.
        ;; __vid_rowaddr preserves BC and DE, so the mask rides in C across it.
        ld      a,e
        and     #0x07
        ld      hl,#.pr_mask_tab
        add     a,l
        ld      l,a
        jr      nc,.pr_mask_hi
        inc     h
.pr_mask_hi:
        ld      c,(hl)

        ;; HL = row base for y (B); __vid_rowaddr preserves DE (x)
        call    __vid_rowaddr
        ld      a,e
        rrca
        rrca
        rrca
        and     #0x1f
        add     a,l
        ld      l,a
        pop     af                      ; A = packed color/mode

        bit     1,a
        jr      nz,.pr_xor
        bit     0,a
        jr      nz,.pr_set

        ;; clear pixel
        ld      a,c
        cpl
        and     (hl)
        ld      (hl),a
        ret

.pr_set:
        ld      a,c
        or      (hl)
        ld      (hl),a
        ret

.pr_xor:
        ld      a,c
        xor     (hl)
        ld      (hl),a
        ret

.pr_reject:
        pop     af
        ret

.pr_mask_tab:
        .db     0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
