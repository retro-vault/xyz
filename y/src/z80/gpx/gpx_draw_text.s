        ;; gpx_draw_text.s
        ;;
        ;; Compact text drawing for ZX Spectrum GPX.
        ;; Walks serialized font_t and renders each glyph via gpx_draw_bmp.
        ;; Supports 1bpp and 1bpp-mask glyph payloads through the bitmap
        ;; renderer (no per-pixel drawing path here).
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_draw_text
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_text
        .globl  _gpx_draw_bmp_clip
        .globl  __gpx_glyph_lookup
        .globl  __gpx_span_setup
        .globl  __gpx_span_row
        .globl  __clip_seg
        .globl  __rect_screen
        .globl  __vid_rowaddr
        .globl  __vid_nextrow
        .globl  __vid_nextrow_carry

        .equ    FONT_FLAG_OFFSETS_BE, 0x02
        .equ    BMP_ENC_MASK,         0xF0
        .equ    BMP_SIG_1BPP,         0x00
        .equ    BMP_SIG_1BPP_MASK,    0x10
        .equ    CO_FORE,              0x01
        .equ    BM_CPY,               0x00

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_text
        ;; Draw a NUL-terminated string with its top-left corner at (x, y).
        ;; Opaque text fills glyph boxes and spacing with the opposite color.
        ;; Transparent text preserves every pixel outside the glyph ink.
        ;; BM_XOR ignores the background policy for the spacing: it inverts
        ;; glyph ink only, so that drawing the same string twice restores
        ;; the display exactly.
        ;;
        ;; Signature:
        ;;   void gpx_draw_text(gpx_t *gpx, coord x, coord y,
        ;;                      const char *text, const font_t *font,
        ;;                      color c, bmode m, const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = x
        ;;   stack: y, text, font, c, m, clip
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL. Preserves IX and IY.
        ;;
        ;; References:
        ;;   __gpx_glyph_lookup
        ;;   _gpx_draw_bmp_clip
        ;;   __gpx_span_setup, __gpx_span_row
_gpx_draw_text::
        push    iy
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; text == NULL ?
        ld      a,8(ix)
        or      9(ix)
        jp      z,.dt_epilogue

        ;; font == NULL ?
        ld      a,10(ix)
        or      11(ix)
        jp      z,.dt_epilogue

        ;; locals (16 bytes):
        ;; -2..-1   xcur (high, low)
        ;; -3       text background policy
        ;; -4       empty_width
        ;; -5       advance
        ;; -6       glyph_height
        ;; -7       visible gap rows (0 for transparent/XOR/empty bands)
        ;; -9..-8   first gap row address (high, low)
        ;; -10,-11  gap x bounds (low, high)
        ;; -16..-12 span descriptor: first/last masks, count, OR/XOR selectors
        ;; Gap bounds and row address are resolved once per string.
        ld      c,#0x00                 ; NULL context defaults to opaque
        ld      a,h
        or      l
        jr      z,.dt_have_background
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl                      ; -> gpx->text_background
        ld      a,(hl)
        and     #0x01
        ld      c,a
.dt_have_background:
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      -3(ix),c

        ;; xcur
        ld      -1(ix),e
        ld      -2(ix),d

        ;; The callee-owned text argument is advanced in place.

        ;; cache only the header fields the drawer itself needs (gap fill):
        ;; empty_width (font[3]), glyph_height (font[5]), advance (font[6]).
        ld      l,10(ix)
        ld      h,11(ix)                ; HL = font
        inc     hl
        inc     hl
        inc     hl                      ; -> font[3]
        ld      a,(hl)
        ld      -4(ix),a                ; empty_width
        inc     hl
        inc     hl                      ; -> font[5]
        ld      a,(hl)
        ld      -6(ix),a                ; glyph_height
        inc     hl
        ld      a,(hl)
        ld      -5(ix),a                ; advance

        call    .dt_gap_band_setup

.dt_loop:
        ;; ch = *text++
        ld      l,8(ix)
        ld      h,9(ix)
        ld      a,(hl)
        or      a
        jr      z,.dt_done
        inc     hl
        ld      8(ix),l
        ld      9(ix),h

        ;; width = __gpx_glyph_lookup(ch, font).  A = ch already; font in DE.
        ;; The helper does the range / offset-table / encoding work itself
        ;; (shared with gpx_measure_text), preserves IX (our frame), and
        ;; returns A = width (0 => missing/empty) and HL = glyph bmp_t*.
        ld      e,10(ix)
        ld      d,11(ix)                ; DE = font
        call    __gpx_glyph_lookup
        or      a
        jr      z,.dt_add_empty
        ld      c,l
        ld      b,h                     ; BC = glyph bmp_t*

        ;; preserve width across draw_bmp call (width in A)
        push    af

        ;; Call the shared bitmap core directly.
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl                      ; clip

        push    bc                      ; glyph bmp_t*

        ld      l,6(ix)
        ld      h,7(ix)
        push    hl                      ; y

        ld      e,-1(ix)                ; (gpx arg unused by the bmp core)
        ld      d,-2(ix)                ; xcur
        ld      b,13(ix)                ; bmode
        ld      a,-3(ix)
        or      a
        jr      z,.dt_have_draw_mode
        set     6,b                     ; transparent text compositor tag
.dt_have_draw_mode:
        ld      c,12(ix)                ; color
        call    _gpx_draw_bmp_clip

        ;; xcur += glyph_width
        pop     af                      ; A = width
        add     a,-1(ix)
        ld      -1(ix),a
        jr      nc,.dt_advance_xcur
        inc     -2(ix)

.dt_advance_xcur:
        ;; Fill inter-character advance gap with inverse color.
        ld      a,-5(ix)                ; advance
.dt_fill_and_advance:
        ;; A = span width; fill the gap, then xcur += width.
        ;; .dt_fill_inv_span draws byte spans (trashes regs), so the width
        ;; is carried across the call on the stack rather than in a register.
        push    af
        call    .dt_fill_inv_span
        pop     af
        add     a,-1(ix)
        ld      -1(ix),a
        jr      nc,.dt_loop
        inc     -2(ix)
        jr      .dt_loop

.dt_add_empty:
        ;; Fill missing/empty glyph span with inverse color.
        ld      a,-4(ix)                ; empty_width
        jr      .dt_fill_and_advance

.dt_done:
        ld      sp,ix

.dt_epilogue:
        pop     ix
        pop     iy

        ;; callee cleanup:
        ;; y(2), text(2), font(2), c(1), m(1), clip(2) = 10
        pop     de
        ld      hl,#10
        add     hl,sp
        ld      sp,hl
        push    de
        ret

.dt_fill_inv_span:
        ;; A = gap width in pixels. Fills [xcur, xcur+A-1] across the band
        ;; that .dt_gap_band_setup resolved once for this string, so nothing
        ;; here re-derives the clip, the row range or the row address.
        or      a
        ret     z
        ld      c,a                     ; gap width
        ld      a,-7(ix)                ; rows visible in the band
        or      a
        ret     z

        ;; --- x0: clamp the 16-bit xcur onto the screen ---
        ld      a,-2(ix)                ; xcur high
        or      a
        jr      z,.dt_gap_x0_lo
        bit     7,a
        ret     z                       ; xcur > 255: nothing visible
        xor     a                       ; xcur < 0: start at 0
        jr      .dt_gap_x0_have
.dt_gap_x0_lo:
        ld      a,-1(ix)
.dt_gap_x0_have:
        ld      b,a                     ; B = x0 on screen

        ;; --- x1 = xcur + width - 1, clamped to 255 ---
        ld      a,c
        dec     a
        add     a,-1(ix)
        ld      c,a
        ld      a,-2(ix)
        adc     a,#0x00
        jr      z,.dt_gap_x1_have
        bit     7,a
        ret     nz                      ; x1 < 0: nothing visible
        ld      c,#255
.dt_gap_x1_have:

        ;; --- narrow to the band's x bounds, all 8-bit from here ---
        ld      a,c
        cp      -10(ix)
        ret     c                       ; x1 < gap_lo
        ld      a,-11(ix)
        cp      b
        ret     c                       ; gap_hi < x0

        ld      a,b
        cp      -10(ix)
        jr      nc,.dt_gap_lo_ok
        ld      a,-10(ix)
        ld      b,a
.dt_gap_lo_ok:
        ld      a,-11(ix)
        cp      c
        jr      nc,.dt_gap_hi_ok
        ld      c,a
.dt_gap_hi_ok:

        ;; --- descriptor, then one solid span per row ---
        ;; IY already points at the descriptor: it is set once per string and
        ;; survives, because gpx_draw_bmp_clip preserves IY and the glyph
        ;; lookup never touches it.
        ld      a,12(ix)                ; text color
        xor     #0x01
        and     #0x01
        ld      d,a                     ; the gap uses the inverse color
        ld      e,#0x00                 ; BM_CPY
        call    __gpx_span_setup        ; A = byte_lo

        ld      e,a
        ld      d,#0x00
        ld      l,-8(ix)
        ld      h,-9(ix)
        add     hl,de                   ; HL = first band row + byte_lo
        ld      b,-7(ix)

        ;; An advance gap is normally one pixel wide, so the whole column
        ;; lands in a single byte. The pattern is solid and the descriptor is
        ;; fixed, so the two plot masks fold once here and each row becomes a
        ;; read-modify-write plus a row step -- no per-row span call at all.
        ;; descriptor is at ix-16: mask_first, mask_last, count,
        ;; sel_or, sel_xor
        ld      a,-14(ix)               ; count
        dec     a
        jr      nz,.dt_gap_wide

        ld      a,-16(ix)               ; mask_first
        and     -15(ix)                 ; & mask_last
        ld      c,a                     ; C = coverage
        and     -13(ix)                 ; & sel_or
        ld      d,a
        ld      a,c
        and     -12(ix)                 ; & sel_xor
        ld      e,a
.dt_gap_byte:
        call    _enter_critical_section
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
        call    _leave_critical_section
        inc     h                       ; inlined __vid_nextrow fast path
        ld      a,h
        and     #0x07
        call    z,__vid_nextrow_carry
        djnz    .dt_gap_byte
        ret

.dt_gap_wide:
.dt_gap_row:
        push    bc
        ld      a,#0xff
        call    __gpx_span_row          ; preserves HL
        call    __vid_nextrow
        pop     bc
        djnz    .dt_gap_row
        ret

        ;; ------------------------------------------------------------
        ;; .dt_gap_band_setup
        ;;
        ;; Resolve everything about the inter-glyph gap band that does not
        ;; depend on x: how many rows of it are visible, where the first one
        ;; lives in VRAM, and the x bounds. Runs once per string.
        ;; Leaves -7 = 0 when no part of the band is visible.
        ;; ------------------------------------------------------------
.dt_gap_band_setup:
        xor     a
        ld      -7(ix),a
        ld      a,-3(ix)
        or      a
        ret     nz                      ; transparent: leave spacing untouched
        ld      a,13(ix)
        rrca
        ret     c                       ; XOR: leave spacing untouched
        ld      a,-6(ix)                ; glyph_height
        or      a
        ret     z

        ;; band = [y, y + glyph_height - 1]
        ld      l,6(ix)
        ld      h,7(ix)
        ld      a,-6(ix)
        dec     a
        add     a,l
        ld      e,a
        ld      a,h
        adc     a,#0x00
        ld      d,a                     ; DE = y1, HL = y0

        ;; against the screen first, then the caller's clip: both are the
        ;; same 1-D clamp, so the shared helper does each in turn.
        ld      iy,#__rect_screen+2
        call    __clip_seg
        ret     c

        ld      a,14(ix)
        or      15(ix)
        jr      z,.dt_band_rows

        ld      c,14(ix)
        ld      b,15(ix)
        push    hl
        ld      hl,#2
        add     hl,bc
        push    hl
        pop     iy                      ; IY = &clip->y0
        pop     hl
        call    __clip_seg
        ret     c

.dt_band_rows:
        ld      a,e
        sub     l
        inc     a
        ld      -7(ix),a                ; visible row count

        ld      b,l
        call    __vid_rowaddr
        ld      -8(ix),l
        ld      -9(ix),h

        ;; x bounds: the screen, narrowed by the clip when there is one
        ld      hl,#0
        ld      de,#255
        ld      a,14(ix)
        or      15(ix)
        jr      z,.dt_band_x_store

        ld      c,14(ix)
        ld      b,15(ix)
        push    hl
        push    bc
        pop     iy                      ; IY = &clip->x0
        pop     hl
        call    __clip_seg
        jr      nc,.dt_band_x_store
        xor     a                       ; clip excludes every column
        ld      -7(ix),a
        ret

.dt_band_x_store:
        ld      -10(ix),l
        ld      -11(ix),e
.dt_band_iy:
        ;; IY = &descriptor, once for the whole string
        push    ix
        pop     iy
        ld      de,#-16
        add     iy,de
        ret
