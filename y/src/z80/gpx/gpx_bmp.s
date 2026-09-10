        ;; gpx_bmp.s
        ;;
        ;; Unified ZX bitmap blitter.
        ;;
        ;; All clipping is resolved up front in 16-bit space, then the
        ;; bitmap is drawn with byte-span composition directly into
        ;; display memory. No pixel fallback path remains here.
        ;;
        ;; Re-entrant/thread-safe: all mutable state lives on the stack.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_bmp
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_bmp
        .globl  _gpx_draw_bmp_clip
        .globl  __gpx_ffshr
        .globl  __rect_cmp16s_lt
        .globl  __vid_rowaddr
        .globl  __vid_nextrow

        .equ    SCRHEIGHT, 192
        .equ    BMP_SIG_ENC_MASK, 0xF0
        .equ    BMP_SIG_1BPP, 0x00
        .equ    BMP_SIG_1BPP_MASK, 0x10

        ;; stack-local bitmap workspace (IY-relative)
        .equ    L_X,                  0
        .equ    L_XHI,                1
        .equ    L_Y,                  2
        .equ    L_YHI,                3
        .equ    L_CLIP,               4
        .equ    L_BPTR,               6
        .equ    L_BW,                 8
        .equ    L_BH,                 9
        .equ    L_BSTRIDE,            10
        .equ    L_ROWSTRIDE_OR,       11
        .equ    L_XEND,               12
        .equ    L_YEND,               14
        .equ    L_VISX0,              16
        .equ    L_VISX1,              17
        .equ    L_VISY0,              18
        .equ    L_VISY1,              19
        .equ    L_VISW,               20
        .equ    L_VISH,               21
        .equ    L_SRCX,               22
        .equ    L_SRCY,               23
        .equ    L_XBYTE,              24
        .equ    L_SRCBYTE,            25
        .equ    L_DBIT,               26
        .equ    L_SRCBIT,             27
        .equ    L_LCOVER,              28
        .equ    L_RCOVER,              29
        .equ    L_SRCSPAN,            30
        .equ    L_DSTSPAN,            31
        .equ    L_SRCROW_OR,          32
        .equ    L_IS_MASKED,          34
        .equ    L_DRAWMODE,           35
        ;; Two-byte source windows keep previous AND/OR bytes in B':C'.
        .equ    L_SUB,                36
        .equ    L_SRCREMAIN,          37
        .equ    L_SIZE,               38

        .macro  LD16HL off
        ld      l,off(iy)
        ld      h,off+1(iy)
        .endm

        .macro  ST16HL off
        ld      off(iy),l
        ld      off+1(iy),h
        .endm

        .macro  LD16DE off
        ld      e,off(iy)
        ld      d,off+1(iy)
        .endm

        .macro  ST16DE off
        ld      off(iy),e
        ld      off+1(iy),d
        .endm

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_draw_bmp
        ;; Blit a bitmap with its top-left corner at (x, y). Public entry:
        ;; the payload's own colours are used and the mode is copy, so it
        ;; sets the internal mode tag and falls into the shared core.
        ;;
        ;; Signature:
        ;;   void gpx_draw_bmp(gpx_t *gpx, coord x, coord y, bmp_t *b,
        ;;                     const rect_t *clip)
        ;;
        ;; Arguments:
        ;;   HL = gpx, DE = x
        ;;   stack: y, b, clip
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   .gb_core
_gpx_draw_bmp::
        ;; Public API semantics:
        ;; - unmasked: draw 1 bits, skip 0 bits
        ;; - masked:   apply AND/OR pair
        ld      b,#0x80                 ; internal mode tag: keep legacy semantics

        ;; ------------------------------------------------------------
        ;; _gpx_draw_bmp_clip
        ;; As gpx_draw_bmp, but the caller chooses the colour and blit
        ;; mode. The text path uses this so the colour and mode given to
        ;; gpx_draw_text actually reach the glyphs.
        ;;
        ;; Arguments:
        ;;   B = bmode, C = color
        ;;   HL = gpx, DE = x
        ;;   stack: y, b, clip
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   .gb_core
_gpx_draw_bmp_clip::
.gb_core:
        ;; Drain y and bitmap-ptr args into the alternate set and park the
        ;; return address back on the stack; clip stays as the only stack
        ;; arg (read at 6(ix) below, discarded by the exit). DE = x and
        ;; BC = mode tags stay live in the main set throughout.
        exx
        pop     bc                      ; BC' = return address
        pop     hl                      ; HL' = y
        pop     de                      ; DE' = bitmap ptr
        push    bc                      ; return address back (clip below it)
        exx
        push    ix
        push    iy
        ld      ix,#0
        add     ix,sp
        ld      hl,#-L_SIZE
        add     hl,sp
        ld      sp,hl
        ld      iy,#0
        add     iy,sp

        ;; Preserve the arguments before IX becomes the compositor address.
        ld      L_X(iy),e
        ld      L_XHI(iy),d
        ld      e,6(ix)
        ld      d,7(ix)
        ST16DE  L_CLIP

        ;; Select the byte compositor once. IX is otherwise unused after
        ;; reading the stack arguments; the saved caller IX remains on the
        ;; stack above the IY-relative workspace. Bit 7 of the incoming mode
        ;; marks public bitmap semantics and is resolved after the header.
        ld      L_DRAWMODE(iy),b
        ld      ix,#.gb_mode_trans_fore
        bit     7,b
        jr      nz,.gb_mode_done
        bit     0,b
        jr      nz,.gb_mode_xor_sel
        bit     6,b
        jr      z,.gb_mode_opaque_select
        bit     0,c
        jr      nz,.gb_mode_done
        ld      ix,#.gb_mode_trans_back
        jr      .gb_mode_done

.gb_mode_opaque_select:
        ld      ix,#.gb_mode_fore
        bit     0,c
        jr      nz,.gb_mode_done
        ld      ix,#.gb_mode_back
        jr      .gb_mode_done

.gb_mode_xor_sel:
        ld      ix,#.gb_mode_xor

.gb_mode_done:
        ;; y and bitmap pointer were drained into the alternate set.
        exx
        ld      L_Y(iy),l
        ld      L_YHI(iy),h
        ST16DE  L_BPTR
        push    de
        exx
        pop     hl                      ; HL = bitmap ptr (main set)

        ;; Null bitmap -> return.
        ld      a,h
        or      l
        jp      z,.gb_exit
        ld      a,(hl)
        and     #BMP_SIG_ENC_MASK
        cp      #BMP_SIG_1BPP
        jr      z,.gb_sig_unmasked
        cp      #BMP_SIG_1BPP_MASK
        jr      z,.gb_sig_masked
        jp      .gb_exit

.gb_sig_unmasked:
        xor     a
        ld      L_IS_MASKED(iy),a
        jr      .gb_sig_need_and

.gb_sig_masked:
        ld      a,#1
        ld      L_IS_MASKED(iy),a

.gb_sig_need_and:
        ;; Only a masked public bitmap reads the AND plane. Every text
        ;; compositor consumes the OR plane, including custom masked fonts.
        ld      a,L_IS_MASKED(iy)
        or      a
        jr      z,.gb_sig_mode_ready
        ld      a,L_DRAWMODE(iy)
        and     #0x80
        jr      z,.gb_sig_mode_ready
        ld      ix,#.gb_mode_masked
.gb_sig_mode_ready:
        ld      L_DRAWMODE(iy),a        ; nonzero only when AND is live

.gb_sig_parse:
        ld      a,(hl)
        and     #0x0F
        inc     a
        ld      L_BSTRIDE(iy),a

        inc     hl
        ld      a,(hl)
        ld      L_BW(iy),a
        inc     hl
        ld      a,(hl)
        ld      L_BH(iy),a

        ld      a,L_BW(iy)
        or      a
        jp      z,.gb_exit
        ld      a,L_BH(iy)
        or      a
        jp      z,.gb_exit

        ;; draw_x1 = x + w - 1
        ld      a,L_BW(iy)
        dec     a
        ld      b,a
        ld      a,L_X(iy)
        add     a,b
        ld      L_XEND(iy),a
        ld      a,L_XHI(iy)
        adc     a,#0
        ld      L_XEND+1(iy),a

        ;; draw_y1 = y + h - 1
        ld      a,L_BH(iy)
        dec     a
        ld      b,a
        ld      a,L_Y(iy)
        add     a,b
        ld      L_YEND(iy),a
        ld      a,L_YHI(iy)
        adc     a,#0
        ld      L_YEND+1(iy),a

        ;; Clamp the draw rect to the screen. The screen bounds are compile
        ;; time constants, so this is a handful of sign and range tests
        ;; instead of copying a screen rect into the workspace and running
        ;; four generic 16-bit clamps against it. The result is on-screen, so
        ;; the high bytes are all zero from here on.

        ;; visx0 = max(x, 0); x > 255 means the bitmap starts past the edge
        ld      a,L_XHI(iy)
        or      a
        jr      z,.gb_sx0_on
        jp      p,.gb_exit
        xor     a                       ; x < 0: start at column 0
        jr      .gb_sx0_store
.gb_sx0_on:
        ld      a,L_X(iy)
.gb_sx0_store:
        ld      L_VISX0(iy),a

        ;; visx1 = min(xend, 255); xend < 0 means it ends before the edge
        ld      a,L_XEND+1(iy)
        or      a
        jr      z,.gb_sx1_on
        jp      m,.gb_exit
        ld      a,#255
        jr      .gb_sx1_store
.gb_sx1_on:
        ld      a,L_XEND(iy)
.gb_sx1_store:
        ld      L_VISX1(iy),a

        ;; visy0 = max(y, 0); y past the bottom means nothing visible
        ld      a,L_YHI(iy)
        or      a
        jr      z,.gb_sy0_on
        jp      p,.gb_exit
        xor     a
        jr      .gb_sy0_store
.gb_sy0_on:
        ld      a,L_Y(iy)
        cp      #SCRHEIGHT
        jp      nc,.gb_exit
.gb_sy0_store:
        ld      L_VISY0(iy),a

        ;; visy1 = min(yend, 191)
        ld      a,L_YEND+1(iy)
        or      a
        jr      z,.gb_sy1_on
        jp      m,.gb_exit
        ld      a,#(SCRHEIGHT-1)
        jr      .gb_sy1_store
.gb_sy1_on:
        ld      a,L_YEND(iy)
        cp      #SCRHEIGHT
        jr      c,.gb_sy1_store
        ld      a,#(SCRHEIGHT-1)
.gb_sy1_store:
        ld      L_VISY1(iy),a

        ;; No clip rect: the on-screen rect is already the visible rect.
        ld      a,L_CLIP(iy)
        or      L_CLIP+1(iy)
        jp      z,.gb_skips

        ;; Narrow by the caller's clip rect, read straight through the
        ;; pointer in rect_t order (x0, y0, x1, y1) so it is walked once.
        ;;
        ;; The visible rect is already on-screen, so each bound is an 8-bit
        ;; value: only the clip's own 16-bit range needs a sign test, and the
        ;; comparison itself is a plain `cp`. That replaces four generic
        ;; 16-bit clamp calls and two 16-bit inversion tests.
        LD16HL  L_CLIP

        ld      e,(hl)                  ; visx0 = max(visx0, clip->x0)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      a,d
        or      a
        jr      z,.gb_cx0_8
        jp      p,.gb_exit              ; clip x0 > 255: nothing visible
        jr      .gb_cy0                 ; clip x0 < 0: visx0 unchanged
.gb_cx0_8:
        ld      a,e
        cp      L_VISX0(iy)
        jr      c,.gb_cy0
        ld      L_VISX0(iy),a

.gb_cy0:
        ld      e,(hl)                  ; visy0 = max(visy0, clip->y0)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      a,d
        or      a
        jr      z,.gb_cy0_8
        jp      p,.gb_exit
        jr      .gb_cx1
.gb_cy0_8:
        ld      a,e
        cp      L_VISY0(iy)
        jr      c,.gb_cx1
        ld      L_VISY0(iy),a

.gb_cx1:
        ld      e,(hl)                  ; visx1 = min(visx1, clip->x1)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      a,d
        or      a
        jr      z,.gb_cx1_8
        jp      m,.gb_exit              ; clip x1 < 0: nothing visible
        jr      .gb_cy1                 ; clip x1 > 255: visx1 unchanged
.gb_cx1_8:
        ld      a,e
        cp      L_VISX1(iy)
        jr      nc,.gb_cy1
        ld      L_VISX1(iy),a

.gb_cy1:
        ld      e,(hl)                  ; visy1 = min(visy1, clip->y1)
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      a
        jr      z,.gb_cy1_8
        jp      m,.gb_exit
        jr      .gb_clip_test
.gb_cy1_8:
        ld      a,e
        cp      L_VISY1(iy)
        jr      nc,.gb_clip_test
        ld      L_VISY1(iy),a

.gb_clip_test:
        ;; nothing visible if either clamp inverted the span
        ld      a,L_VISX1(iy)
        cp      L_VISX0(iy)
        jp      c,.gb_exit
        ld      a,L_VISY1(iy)
        cp      L_VISY0(iy)
        jp      c,.gb_exit

.gb_skips:
        ;; Source skip is the clipped-away left/top part.
        ld      a,L_VISX0(iy)           ; srcx = visx0 - x
        sub     L_X(iy)
        ld      L_SRCX(iy),a

        ld      a,L_VISY0(iy)           ; srcy = visy0 - y
        sub     L_Y(iy)
        ld      L_SRCY(iy),a

        ;; visw = visx1 - visx0 + 1
        ld      a,L_VISX1(iy)
        sub     L_VISX0(iy)
        inc     a
        ld      L_VISW(iy),a
        or      a
        jp      z,.gb_exit

        ;; vish = visy1 - visy0 + 1
        ld      a,L_VISY1(iy)
        sub     L_VISY0(iy)
        inc     a
        ld      L_VISH(iy),a
        or      a
        jp      z,.gb_exit

        ;; Destination x byte / bit.
        ld      a,L_VISX0(iy)
        ld      b,a
        and     #0x07
        ld      L_DBIT(iy),a
        ld      c,a                     ; dbit rides in C through this block
        ld      a,b
        rrca
        rrca
        rrca
        and     #0x1f
        ld      L_XBYTE(iy),a

        ;; Source x byte / bit.
        ld      a,L_SRCX(iy)
        ld      b,a
        and     #0x07
        ld      L_SRCBIT(iy),a
        ld      e,a                     ; srcbit rides in E
        ld      a,b
        rrca
        rrca
        rrca
        and     #0x1f
        ld      L_SRCBYTE(iy),a

        ;; rshift = (dbit - srcbit) & 7
        ld      a,c
        sub     e
        and     #0x07

        ;; sub = (8 - rshift) & 7  (left shift used by the 2-byte src window)
        neg
        and     #0x07
        ld      L_SUB(iy),a

        ;; First-byte coverage is 0xff >> dbit, including dbit = 0.
        ld      a,c
        call    __gpx_ffshr
        ld      L_LCOVER(iy),a

        ;; Last-byte coverage is the inverse right-edge preservation mask.
        ld      a,L_VISW(iy)
        add     a,c
        and     #0x07
        jr      z,.gb_rmask_zero
        call    __gpx_ffshr
.gb_rmask_zero:
        cpl
        ld      L_RCOVER(iy),a

.gb_rmask_done:
        ;; srcspan = (srcbit + visw + 7) >> 3 ; dstspan = (dbit + visw + 7) >> 3
        ld      a,e
        call    .gb_span
        ld      L_SRCSPAN(iy),a
        ld      a,c
        call    .gb_span
        ld      L_DSTSPAN(iy),a

        ;; Walk to the first visible source row in registers and retain
        ;; only its OR pointer. The AND plane is one stride before it.
        LD16HL  L_BPTR
        ld      de,#5
        add     hl,de
        ld      e,L_BSTRIDE(iy)
        ld      d,#0
        ld      a,L_IS_MASKED(iy)
        or      a
        jr      z,.gb_rows_stride
        add     hl,de                   ; OR follows the first AND stride
        sla     e                       ; masked rows contain both planes
.gb_rows_stride:
        ld      L_ROWSTRIDE_OR(iy),e
        ld      b,L_SRCY(iy)
        inc     b
        dec     b
        jr      z,.gb_rows_skip_x
.gb_rows_skip_loop:
        add     hl,de
        djnz    .gb_rows_skip_loop
.gb_rows_skip_x:
        ld      e,L_SRCBYTE(iy)
        add     hl,de
        ST16HL  L_SRCROW_OR

.gb_rows_ready:
        ;; The destination row address is derived once here and then stepped:
        ;; consecutive display rows are one __vid_nextrow apart, which is far
        ;; cheaper than rebuilding the interleaved address from y every row.
        ;; The row base low byte is a multiple of 0x20 and xbyte is 0..31, so
        ;; the add cannot carry.
        ld      b,L_VISY0(iy)
        call    __vid_rowaddr
        ld      a,L_XBYTE(iy)
        add     a,l
        ld      l,a                     ; HL now rides through the row loop

.gb_row_loop:
        ;; Source pointers live in the alternate bank. Derive AND from OR
        ;; only when it is read, so the row loop advances just one pointer.
        exx
        LD16DE  L_SRCROW_OR
        ld      a,L_DRAWMODE(iy)
        or      a
        jr      z,.gb_row_ptrs_done
        ld      h,d
        ld      l,e
        ld      c,L_BSTRIDE(iy)
        ld      b,#0
        sbc     hl,bc                   ; OR test above cleared carry
.gb_row_ptrs_done:
        exx

        ;; per-row: reset the source counter, prime the window prev bytes
        ld      a,L_SRCSPAN(iy)
        ld      L_SRCREMAIN(iy),a

        ;; hb0 = (dbit > srcbit) ? -1 : 0
        ld      a,L_SRCBIT(iy)
        cp      L_DBIT(iy)              ; C if srcbit < dbit => dbit>srcbit => hb0=-1
        jr      c,.gb_prime_default

        ;; hb0 = 0: prev = first source byte (consume one from each live plane)
        ld      a,L_DRAWMODE(iy)
        or      a
        jr      z,.gb_prime_or_only
        exx
        ld      a,(hl)                  ; AND plane (HL')
        inc     hl
        ld      b,a                     ; B' = previous AND byte
        exx
.gb_prime_or_only:
        exx
        ld      a,(de)                  ; OR plane (DE')
        inc     de
        ld      c,a                     ; C' = prevO for the fast path
        exx
        dec     L_SRCREMAIN(iy)
        jr      .gb_dst_init

.gb_prime_default:
        ;; hb0 = -1: transparent previous AND and OR bytes.
        exx
        ld      bc,#0xff00
        exx

.gb_dst_init:

        ;; The left and right edge masks only ever apply to the first and
        ;; last byte of the span, so the run is walked in three phases and
        ;; the middle bytes carry no edge handling at all. The destination
        ;; byte stays addressed by HL and is read through (hl) at 7 T-states.
        ld      a,L_DSTSPAN(iy)
        dec     a
        jr      z,.gb_span_single

        ;; --- first byte ---
        ld      a,L_LCOVER(iy)
        ld      c,a                     ; destination coverage mask
        call    .gb_byte

        ;; --- middle bytes, entirely inside the visible span ---
        ld      a,L_DSTSPAN(iy)
        sub     #2
        jr      z,.gb_span_last
        ld      b,a
        ld      c,#0xff                 ; middle bytes are fully covered
.gb_mid_loop:
        call    .gb_byte
        djnz    .gb_mid_loop

.gb_span_last:
        ld      a,L_RCOVER(iy)
        ld      c,a                     ; destination coverage mask
        call    .gb_byte
        jp      .gb_next_row

.gb_span_single:
        ld      a,L_LCOVER(iy)
        and     L_RCOVER(iy)
        ld      c,a                     ; destination coverage mask
        call    .gb_byte
        jp      .gb_next_row

        ;; ------------------------------------------------------------
        ;; .gb_byte: compose one destination byte at (HL) and advance HL.
        ;; All state is in the caller's stack frame via IY, so this stays
        ;; re-entrant. C supplies coverage; BC is preserved. Clobbers A, DE
        ;; and the alternate bank.
        ;;
        ;; The AND plane is live only for a masked bitmap drawn with the
        ;; default public semantics. Every other draw leaves HL' free, so the
        ;; two-byte source window can shift there with add hl,hl and the
        ;; previous source byte can stay in C' instead of the workspace.
        ;; ------------------------------------------------------------
.gb_byte:
        ld      a,L_DRAWMODE(iy)
        or      a
        jp      nz,.gb_byte_masked

        ld      a,L_SRCREMAIN(iy)
        or      a
        jr      z,.gb_fast_pad
        dec     L_SRCREMAIN(iy)
        exx
        ld      a,(de)                  ; curO from the OR pointer in DE'
        inc     de
        jr      .gb_fast_have
.gb_fast_pad:
        exx
        xor     a                       ; past the source span: zero fill
.gb_fast_have:
        ld      h,c                     ; H = prevO
        ld      l,a                     ; L = curO
        ld      c,a                     ; C' = prevO for the next byte
        ld      b,L_SUB(iy)             ; IY is not swapped by exx
        inc     b
        dec     b
        jr      z,.gb_fast_done
.gb_fast_lp:
        add     hl,hl                   ; 11T a step, against 16T in D:E
        djnz    .gb_fast_lp
.gb_fast_done:
        ld      a,h
        exx
        ld      e,a                     ; E = ORBITS, straight into the compose
        jr      .gb_have_src

.gb_byte_masked:
        ;; Previous source bytes stay in B':C'. Two stack transfers bring
        ;; the old/current pairs into DE and BC without indexed byte state.
        push    bc                      ; byte counter and coverage mask
        push    hl                      ; destination
        exx
        push    bc                      ; previous AND:OR
        ld      a,L_SRCREMAIN(iy)
        or      a
        jr      z,.gb_cur_default
        dec     L_SRCREMAIN(iy)
        ld      a,(hl)
        inc     hl
        ld      b,a
        ld      a,(de)
        inc     de
        ld      c,a
        jr      .gb_masked_windows
.gb_cur_default:
        ld      bc,#0xff00              ; trailing transparent source
.gb_masked_windows:
        push    bc                      ; current AND:OR, retained in B':C'
        exx
        pop     bc
        pop     de
        ld      h,e                     ; previous OR
        ld      l,c                     ; current OR
        ld      e,b                     ; current AND; D = previous AND
        ld      b,L_SUB(iy)
        ld      a,b
        or      a
        jr      z,.gb_masked_shifted
.gb_masked_shift:
        sla     e
        rl      d
        add     hl,hl
        djnz    .gb_masked_shift
.gb_masked_shifted:
        ld      e,h                     ; OR bits; D retains AND bits
        pop     hl
        pop     bc

.gb_have_src:
        jp      (ix)                    ; compositor chosen once per bitmap

.gb_mode_fore:
        ;; FORE copy: write source bits directly.
        ld      a,e
        jr      .gb_store_out

.gb_mode_masked:
        ;; Masked source, public semantics: draw = (old & AND) | OR.
        ld      a,d
        and     (hl)
        or      e
        jr      .gb_store_out

.gb_mode_back:
        ;; BACK copy: write inverted source bits directly.
        ld      a,e
        cpl
        jr      .gb_store_out

.gb_mode_trans_fore:
        ;; Set only covered source bits; zeroes and uncovered bits survive.
        ld      a,e
        and     c
        or      (hl)
        jr      .gb_store_byte

.gb_mode_trans_back:
        ;; Transparent BACK copy: clear source bits, preserve source zeroes.
        ld      a,e
        and     c
        cpl
        and     (hl)
        jr      .gb_store_byte

.gb_mode_xor:
        ;; XOR: flip OR bits only. old ^ ((old ^ (old ^ src)) & inside)
        ;; collapses to old ^ (src & inside), so this path skips the merge.
        ld      a,e
        and     c
        xor     (hl)
        jr      .gb_store_byte

.gb_store_out:
        ;; A = draw; out = old ^ ((old ^ draw) & inside)
        xor     (hl)
        and     c
        xor     (hl)
.gb_store_byte:
        ld      (hl),a                  ; HL = DSTPTR
        inc     hl                      ; advance DSTPTR (kept in HL)
        ret

.gb_next_row:
        dec     L_VISH(iy)
        jr      z,.gb_exit

        ;; Advance the one saved source pointer by the complete row stride.
        ld      a,L_ROWSTRIDE_OR(iy)
        add     a,L_SRCROW_OR(iy)
        ld      L_SRCROW_OR(iy),a
        jr      nc,.gb_nr_dst
        inc     L_SRCROW_OR+1(iy)
.gb_nr_dst:
        ;; The byte phases left HL one past the end of the span, so rewind by
        ;; the span width and step a row: no workspace round-trip either way.
        ld      a,l
        sub     L_DSTSPAN(iy)
        ld      l,a
        jr      nc,.gb_nr_step
        dec     h
.gb_nr_step:
        call    __vid_nextrow
        jp      .gb_row_loop

.gb_exit:
        ;; y and b were popped at entry; drop the remaining clip word.
        ld      sp,iy
        ld      hl,#L_SIZE
        add     hl,sp
        ld      sp,hl
        pop     iy
        pop     ix
        pop     de                      ; return address
        pop     hl                      ; discard clip
        push    de
        ret

        ;; Row address (y in B -> HL) is shared with _video.s (__vid_rowaddr).

        ;; .gb_span: A = bit (0..7) -> A = (bit + L_VISW + 7) >> 3
.gb_span:
        ;; bit+7 fits a byte; adding the width needs at most nine bits.
        ;; RRA consumes that ninth bit before the remaining two rotations.
        add     a,#7
        add     a,L_VISW(iy)
        rra
        rrca
        rrca
        and     #0x3f
        ret
