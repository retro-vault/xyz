        ;; gpx_measure_text.s
        ;;
        ;; Measure text width from serialized font_t data.
        ;; No decoded meta structure is used; the routine reads
        ;; font header fields directly from font_t.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_measure_text
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_measure_text
        .globl  __gpx_glyph_lookup

        .equ    FONT_FLAG_OFFSETS_BE, 0x02
        .equ    BMP_ENC_MASK,         0xF0
        .equ    BMP_SIG_1BPP,         0x00
        .equ    BMP_SIG_1BPP_MASK,    0x10

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_measure_text
        ;; Width in pixels the string would occupy if drawn, without
        ;; drawing anything. Used to centre or right-align text.
        ;;
        ;; Signature:
        ;;   coord gpx_measure_text(const char *text, const font_t *font)
        ;;
        ;; Arguments:
        ;;   HL = text, DE = font
        ;;
        ;; Return:
        ;;   DE = width in pixels; zero for a NULL text or font
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, and the alternate set
        ;;
        ;; References:
        ;;   __gpx_glyph_lookup
_gpx_measure_text::
        ld      a,h                     ; text != NULL ?
        or      l
        jr      z,.mt_zero

        ld      a,d                     ; font != NULL ?
        or      e
        jr      z,.mt_zero

        push    ix
        push    de
        pop     ix                      ; IX = font
        push    hl
        exx
        pop     hl                      ; HL' = text (survives the lookup)
        exx

        ld      bc,#0x0000              ; BC = accumulated width

.mt_loop:
        exx                             ; ch = *text++ from the alt bank
        ld      a,(hl)
        or      a
        inc     hl
        exx                             ; flags survive exx
        jr      z,.mt_done

        ;; width = __gpx_glyph_lookup(ch, font).  IX = font here, so pass
        ;; font in DE; the helper preserves IX and clobbers BC (the width
        ;; accumulator), so save it across the call.
        push    ix
        pop     de                      ; DE = font
        push    bc
        call    __gpx_glyph_lookup      ; A = width (0 => empty), HL = glyph
        pop     bc
        or      a
        jr      z,.mt_add_empty

        ;; sum += glyph_width + advance
        add     a,c
        ld      c,a
        jr      nc,.mt_adv
        inc     b
.mt_adv:
        ld      a,c
        add     a,6(ix)                 ; advance
        ld      c,a
        jr      nc,.mt_loop
        inc     b
        jr      .mt_loop

.mt_add_empty:
        ld      a,c
        add     a,3(ix)                 ; empty_width
        ld      c,a
        jr      nc,.mt_loop
        inc     b
        jr      .mt_loop

.mt_done:
        ld      d,b
        ld      e,c
        pop     ix
        ret

.mt_zero:
        ld      de,#0x0000
        ret

        ;; ------------------------------------------------------------
        ;; __gpx_glyph_lookup  (shared by gpx_measure_text + gpx_draw_text)
        ;;
        ;; Resolve a character to its glyph in the serialized (frozen) font.
        ;;   IN:  A  = character
        ;;        DE = font base pointer
        ;;   OUT: A  = glyph width in pixels (0 => missing/empty: use empty_width)
        ;;        HL = glyph bmp_t* (valid only when A != 0)
        ;; Reads the same header fields + offset table (incl FONT_FLAG_OFFSETS_BE)
        ;; as the old inline code. Clobbers AF, BC, DE, HL. Preserves IX, IY.
        ;; ------------------------------------------------------------
__gpx_glyph_lookup:
        ;; Walk the header with HL; BC retains the font base for the table
        ;; and glyph additions. Neither index register needs a frame.
        ld      h,d
        ld      l,e
        ld      c,a                     ; character
        inc     hl                      ; first
        sub     (hl)
        jr      c,.gl_empty
        ld      b,a                     ; index = character - first
        inc     hl                      ; last
        ld      a,(hl)
        cp      c
        jr      c,.gl_empty

        ld      l,b
        ld      h,#0
        ld      b,d
        ld      c,e                     ; BC = font
        add     hl,hl
        add     hl,de
        ld      de,#8
        add     hl,de                   ; HL = &offset[index]

        ld      a,(bc)                  ; font flags
        and     #FONT_FLAG_OFFSETS_BE
        jr      nz,.gl_be
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        jr      .gl_have_off
.gl_be:
        ld      d,(hl)
        inc     hl
        ld      e,(hl)
.gl_have_off:
        ld      a,d
        or      e
        ret     z                       ; offset zero means missing

        ld      h,b
        ld      l,c
        add     hl,de                   ; HL = glyph base
        ld      a,(hl)
        and     #0xe0                   ; encodings 0x00 and 0x10 are valid
        jr      nz,.gl_empty
        inc     hl
        ld      a,(hl)                  ; zero width also means empty
        dec     hl
        ret

.gl_empty:
        xor     a
        ret
