        ;; _gpx_hline.s
        ;;
        ;; Fast horizontal line helper:
        ;;  - assumes x0 <= x1 (sorted)
        ;;  - quick clip reject against the clip rectangle, or the screen when
        ;;    the caller passes no clip
        ;;  - clips x to those bounds
        ;;  - draws by bytes (start/stop masks + full middle bytes)
        ;;  - preserves pattern phase (skip + drawn span)
        ;;  - pattern 0-bits are left alone, matching gpx_draw_pixel and the
        ;;    Bresenham raster: CO_FORE sets pattern-1 pixels, CO_BACK clears
        ;;    them, BM_XOR toggles them and ignores the color.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _gpx_hline
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_hline
        .globl  __gpx_span_row
        .globl  __gpx_span_setup
        .globl  __rect_cmp16s_lt
        .globl  __ret_clean11
        .globl  __clip_seg
        .globl  __vid_rowaddr

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __gpx_hline
        ;;
        ;; Clipped wrapper:
        ;;  - optional clip reject/clamp against clip rectangle
        ;;  - then enters shared draw core
        ;;
        ;; Signature is identical to gpx_draw_line():
        ;;   HL = gpx
        ;;   DE = x0
        ;;   stack: y0, x1, y1, c, m, lpatt, clip
        ;;
        ;; Return:
        ;;   A = updated lpatt, or unchanged lpatt on reject.
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   __gpx_span_setup
        ;;   __gpx_span_row
        ;;   __vid_rowaddr
__gpx_hline::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; locals (13 bytes):
        ;; -1..-2  x0
        ;; -3..-4  x1
        ;; -5      x0 original low
        ;; -6      unused
        ;; -7      patt_byte
        ;; -8      byte_lo
        ;; -13..-9 span descriptor, laid out for __gpx_span_row:
        ;;         -13 mask_first, -12 mask_last, -11 count,
        ;;         -10 sel_or,     -9 sel_xor
        ld      hl,#-13
        add     hl,sp
        ld      sp,hl

        ;; cache x0/x1
        ld      -2(ix),e                ; x0 lo
        ld      -1(ix),d                ; x0 hi
        ld      -5(ix),e                ; x0 original lo
        ld      a,6(ix)
        ld      -4(ix),a                ; x1 lo
        ld      a,7(ix)
        ld      -3(ix),a                ; x1 hi

        ;; The screen is an implicit clip, and it is applied first: a clip
        ;; rect may itself extend off-screen, and a byte-span path handed a
        ;; span the screen cannot hold would write past the display file.
        ld      a,5(ix)                 ; y hi: nonzero => y < 0 or y > 255
        or      a
        jp      nz,.ghl_reject
        ld      a,4(ix)
        cp      #192
        jp      nc,.ghl_reject

        ld      a,-3(ix)                ; x1 hi
        or      a
        jr      z,.ghl_x1_onscreen
        jp      m,.ghl_reject           ; x1 < 0: nothing visible
        ld      a,#255                  ; x1 > 255: clamp
        ld      -4(ix),a
        xor     a
        ld      -3(ix),a
.ghl_x1_onscreen:
        ld      a,-1(ix)                ; x0 hi
        or      a
        jr      z,.ghl_x0_onscreen
        jp      p,.ghl_reject           ; x0 > 255: nothing visible
        xor     a                       ; x0 < 0: clamp
        ld      -1(ix),a
        ld      -2(ix),a
.ghl_x0_onscreen:

        ;; optional caller clip, on top of the screen bounds
        ld      a,13(ix)
        or      14(ix)
        jr      z,.ghl_draw_core

.ghl_clip_checks:
        ;; BC = clip pointer
        ld      c,13(ix)
        ld      b,14(ix)

        ;; y outside the clip's y-range? A point is a degenerate segment,
        ;; so the shared 1-D clip does the reject test (clamp is a no-op).
        ld      iy,#2
        add     iy,bc                   ; IY = &clip->y0 (y1 at IY+4)
        ld      l,4(ix)
        ld      h,5(ix)                 ; HL = y
        ld      e,l
        ld      d,h                     ; DE = y
        call    __clip_seg
        jp      c,.ghl_reject
        ;; __clip_seg preserves IY at &clip->y0. Move back to x0
        ;; without reloading the clip pointer and rebuilding IY.
        dec     iy
        dec     iy
        ld      l,-2(ix)
        ld      h,-1(ix)                ; HL = x0
        ld      e,-4(ix)
        ld      d,-3(ix)                ; DE = x1
        call    __clip_seg
        jp      c,.ghl_reject
        ld      -2(ix),l
        ld      -1(ix),h                ; x0 = clamped lo
        ld      -4(ix),e
        ld      -3(ix),d                ; x1 = clamped hi

.ghl_draw_core:
        ;; Clipping changes neither the pattern's original byte-grid phase
        ;; nor its origin. Reverse the original LSB-first line pattern,
        ;; then rotate right by original x0, independent of the clipped x0.
        ld      a,12(ix)
        cp      #0xFF
        jr      z,.ghl_pbyte_store      ; solid rows need no phase conversion
        ld      b,#8
.ghl_rev_loop:
        rrca
        rl      d
        djnz    .ghl_rev_loop

        ld      a,-5(ix)
        and     #7
        ld      b,a
        ld      a,d
        jr      z,.ghl_pbyte_store
.ghl_pbyte_rot:
        rrca
        djnz    .ghl_pbyte_rot
.ghl_pbyte_store:
        ld      -7(ix),a

        ;; Span descriptor for __gpx_span_row: IY = &descriptor (ix - 13).
        push    ix
        pop     iy
        ld      de,#-13
        add     iy,de

        ld      b,-2(ix)                ; x0 (clamped to the screen)
        ld      c,-4(ix)                ; x1
        ld      d,10(ix)                ; color
        ld      e,11(ix)                ; mode
        call    __gpx_span_setup        ; A = byte_lo
        ld      -8(ix),a

        ;; HL = row base + byte_lo
        ld      b,4(ix)                 ; y low
        call    __vid_rowaddr
        ld      a,-8(ix)
        add     a,l
        ld      l,a
        ld      a,-7(ix)                ; byte-grid aligned pattern
        call    __gpx_span_row

.ghl_ret_pattern:
        ;; Return phase includes both clipped-away and visible pixels.
        ld      a,-4(ix)
        sub     -5(ix)
        and     #0x07                   ; Z set when shift==0, A=shift
        ld      b,a                     ; B = rotate count
        ld      a,12(ix)                ; original pattern
        jr      z,.ghl_return
.ghl_pret_rot:
        rrca
        djnz    .ghl_pret_rot
        jr      .ghl_return

.ghl_reject:
        ;; unchanged lpatt on reject
        ld      a,12(ix)

.ghl_return:
        ld      sp,ix
        pop     ix
        jp      __ret_clean11

        ;; ------------------------------------------------------------
        ;; __gpx_span_setup
        ;;
        ;; Fill the span descriptor __gpx_span_row consumes. Everything here
        ;; is constant down a filled rectangle, which is why it is a separate
        ;; step: gpx_fill_rectangle calls it once and then loops rows.
        ;;
        ;;   B  = x0, C = x1   both already clamped to 0..255, x0 <= x1
        ;;   D  = color byte, E = mode byte
        ;; Arguments:
        ;;   B  = x0, C = x1, both clamped to 0..255 with x0 <= x1
        ;;   D  = color byte, E = mode byte
        ;;   IY = descriptor (+0 mask_first, +1 mask_last, +2 count,
        ;;                    +3 sel_or, +4 sel_xor)
        ;;
        ;; Return:
        ;;   A = byte_lo (x0 >> 3)
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, L
        ;; ------------------------------------------------------------
__gpx_span_setup::
        ;; mask_first = 0xFF >> (x0 & 7)
        ld      a,b
        and     #0x07
        ld      l,a
        ld      a,#0xff
        jr      z,.ss_first_done
.ss_first_lp:
        srl     a
        dec     l
        jr      nz,.ss_first_lp
.ss_first_done:
        ld      0(iy),a

        ;; mask_last = 0xFF << (7 - (x1 & 7))
        ld      a,c
        cpl
        and     #0x07
        ld      l,a
        ld      a,#0xff
        jr      z,.ss_last_done
.ss_last_lp:
        add     a,a
        dec     l
        jr      nz,.ss_last_lp
.ss_last_done:
        ld      1(iy),a

        ;; count = (x1 >> 3) - (x0 >> 3) + 1
        ld      a,b
        rrca
        rrca
        rrca
        and     #0x1f
        ld      l,a                     ; byte_lo
        ld      a,c
        rrca
        rrca
        rrca
        and     #0x1f
        sub     l
        inc     a
        ld      2(iy),a

        ;; Plot selectors: set FF/00, clear FF/FF, xor 00/FF. Pattern 0-bits
        ;; never reach the destination in any of them.
        ld      a,e                     ; mode
        bit     0,a
        ld      b,#0xff
        ld      c,#0x00
        jr      z,.ss_cpy
        ld      b,#0x00                 ; BM_XOR ignores the color
        ld      c,#0xff
        jr      .ss_store
.ss_cpy:
        ld      a,d                     ; color
        bit     0,a
        jr      nz,.ss_store            ; CO_FORE: set
        ld      c,#0xff                 ; CO_BACK: clear
.ss_store:
        ld      3(iy),b
        ld      4(iy),c
        ld      a,l                     ; A = byte_lo
        ret

        ;; ------------------------------------------------------------
        ;; __gpx_span_row
        ;;
        ;; Apply one byte-aligned pattern across a horizontal run of bytes.
        ;; Every value that is constant for the run lives in the descriptor,
        ;; so a caller filling many rows pays the setup once instead of once
        ;; per row.
        ;;
        ;;   HL = first destination byte of the run
        ;;   A  = pattern byte, already rotated onto the byte grid
        ;;   IY = span descriptor:
        ;;        +0 mask_first  coverage of the first byte
        ;;        +1 mask_last   coverage of the last byte
        ;;        +2 count       bytes in the run, >= 1
        ;;        +3 sel_or      plot selectors, see below
        ;;        +4 sel_xor
        ;;
        ;; Each byte becomes  dest = (dest | (m & sel_or)) ^ (m & sel_xor)
        ;; with m = pattern & coverage: set FF/00, clear FF/FF, xor 00/FF.
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE. HL, IX and IY are preserved, so a caller filling
        ;;   many rows keeps the row pointer in HL and simply steps it.
        ;; ------------------------------------------------------------
__gpx_span_row::
        push    hl
        call    .sr_run
        pop     hl
        ret

.sr_run:
        ld      c,a                     ; C = pattern, live for the whole run
        ld      b,2(iy)                 ; B = byte count
        dec     b
        jr      z,.sr_single

        ld      a,0(iy)
        and     c
        call    .sr_apply

        dec     b
        jr      z,.sr_last

        ;; Middle bytes are fully covered, so the selectors fold into the
        ;; pattern once for the whole run instead of once per byte.
        ld      a,c
        and     3(iy)
        ld      d,a
        ld      a,c
        and     4(iy)
        ld      e,a
        ld      a,d
        inc     a
        jr      nz,.sr_mid_loop
        ld      a,e
        cpl                             ; solid set/clear: output is FF ^ E
.sr_mid_store:
        inc     hl
        ld      (hl),a
        djnz    .sr_mid_store
        jr      .sr_last
.sr_mid_loop:
        inc     hl
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
        djnz    .sr_mid_loop

.sr_last:
        inc     hl
        ld      a,1(iy)
        and     c
        jr      .sr_apply               ; tail: its ret goes to our caller

.sr_single:
        ld      a,0(iy)
        and     1(iy)
        and     c
        ;; fall through

.sr_apply:
        ;; A = m
        ld      e,a
        and     3(iy)
        or      (hl)
        ld      d,a
        ld      a,e
        and     4(iy)
        xor     d
        ld      (hl),a
        ret
