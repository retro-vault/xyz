        ;; console.s -- ZX Spectrum bitmap console
        ;;
        ;; Adapted from the YOS TTY/video routines. It renders the proportional
        ;; 6x12 Tamsyn-derived snatch font directly into the Spectrum display.

        .module zx_console
        .optsdcc -mz80 sdcccall(1)

        .globl  _zx_console_init
        .globl  _zx_console_putc_a
        .globl  _zx_console_scroll
        .globl  _zx_tamsyn

ZX_BITMAP      .equ    0x4000
ZX_ATTRIBUTES  .equ    0x5800
ZX_FONT_FIRST  .equ    32
ZX_FONT_LAST   .equ    127
ZX_FONT_HEIGHT .equ    12
ZX_LAST_ROW    .equ    15

        .area   _CODE

;; Clear the bitmap, select white ink on black paper, and home the cursor.
_zx_console_init::
        xor     a
        out     (0xfe),a
        ld      hl,#ZX_BITMAP
        ld      de,#ZX_BITMAP+1
        ld      bc,#0x17ff
        ld      (hl),#0
        ldir
        ld      hl,#ZX_ATTRIBUTES
        ld      de,#ZX_ATTRIBUTES+1
        ld      bc,#0x02ff
        ld      (hl),#0x07
        ldir
        xor     a
        ld      (_zx_console_x),a
        ld      (_zx_console_row),a
        ret

;; A = character. LF advances a line; CR homes the current line.
_zx_console_putc_a::
        cp      #10
        jp      z,.zx_console_newline
        cp      #13
        jr      nz,.zx_console_printable
        xor     a
        ld      (_zx_console_x),a
        ret

.zx_console_printable:
        cp      #ZX_FONT_FIRST
        ret     c
        cp      #ZX_FONT_LAST
        jr      c,.zx_console_char_ok
        ld      a,#'?'
.zx_console_char_ok:
        sub     #ZX_FONT_FIRST
        ld      e,a
        ld      d,#0
        sla     e
        rl      d
        ld      hl,#_zx_tamsyn+8
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      hl,#_zx_tamsyn
        add     hl,de
        inc     hl
        ld      a,(hl)
        ld      (_zx_console_glyph_width),a
        or      a
        jr      nz,.zx_console_have_width
        ld      a,#3
.zx_console_have_width:
        ld      (_zx_console_advance_width),a

        ;; Wrap before a glyph that would cross the right edge.
        ld      b,a
        ld      a,(_zx_console_x)
        add     a,b
        inc     a
        jr      nc,.zx_console_position_ok
        push    hl
        call    .zx_console_newline
        pop     hl
.zx_console_position_ok:
        ld      a,(_zx_console_glyph_width)
        or      a
        jp      z,.zx_console_advance

        ;; HL points at width. Skip height and the two-byte payload size.
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        ld      (_zx_console_glyph_ptr),hl

        ;; Select the left-aligned erase mask for widths 1..6.
        ld      a,(_zx_console_glyph_width)
        ld      c,a
        ld      b,#0
        ld      hl,#.zx_console_masks
        add     hl,bc
        ld      a,(hl)
        ld      (_zx_console_mask),a

        ;; Convert the logical row to its first pixel scanline.
        ld      a,(_zx_console_row)
        add     a,a
        add     a,a
        ld      b,a
        add     a,a
        add     a,b
        ld      (_zx_console_draw_y),a
        ld      a,#ZX_FONT_HEIGHT
        ld      (_zx_console_rows_left),a

.zx_console_row_loop:
        ld      hl,(_zx_console_glyph_ptr)
        ld      a,(hl)
        inc     hl
        ld      (_zx_console_glyph_ptr),hl
        ld      (_zx_console_bits),a

        ld      a,(_zx_console_draw_y)
        ld      b,a
        call    .zx_console_row_address

        ld      a,(_zx_console_x)
        ld      c,a
        and     #7
        ld      (_zx_console_shift),a
        ld      a,c
        srl     a
        srl     a
        srl     a
        ld      (_zx_console_byte_x),a
        ld      c,a
        ld      b,#0
        add     hl,bc

        ld      a,(_zx_console_mask)
        ld      b,a
        ld      a,(_zx_console_bits)
        ld      d,a
        ld      c,#0
        ld      e,#0
        ld      a,(_zx_console_shift)
        or      a
        jr      z,.zx_console_shifted
.zx_console_shift_loop:
        srl     b
        rr      c
        srl     d
        rr      e
        dec     a
        jr      nz,.zx_console_shift_loop
.zx_console_shifted:
        ld      a,b
        cpl
        and     (hl)
        or      d
        ld      (hl),a
        ld      a,(_zx_console_byte_x)
        cp      #31
        jr      z,.zx_console_next_row
        inc     hl
        ld      a,c
        cpl
        and     (hl)
        or      e
        ld      (hl),a

.zx_console_next_row:
        ld      a,(_zx_console_draw_y)
        inc     a
        ld      (_zx_console_draw_y),a
        ld      a,(_zx_console_rows_left)
        dec     a
        ld      (_zx_console_rows_left),a
        jr      nz,.zx_console_row_loop

.zx_console_advance:
        ld      a,(_zx_console_x)
        ld      b,a
        ld      a,(_zx_console_advance_width)
        add     a,b
        jr      c,.zx_console_advance_newline
        inc     a
        ld      (_zx_console_x),a
        ret     nz
.zx_console_advance_newline:
        jp      .zx_console_newline

.zx_console_newline:
        xor     a
        ld      (_zx_console_x),a
        ld      a,(_zx_console_row)
        cp      #ZX_LAST_ROW
        jr      z,.zx_console_scroll_line
        inc     a
        ld      (_zx_console_row),a
        ret
.zx_console_scroll_line:
        call    _zx_console_scroll
        ret

;; Scroll the bitmap by twelve pixel scanlines. Attributes are uniform.
_zx_console_scroll::
        ld      a,#ZX_FONT_HEIGHT
        ld      (_zx_console_scroll_y),a
.zx_console_scroll_loop:
        ld      a,(_zx_console_scroll_y)
        cp      #192
        jr      z,.zx_console_clear_tail
        ld      b,a
        call    .zx_console_row_address
        ex      de,hl
        ld      a,(_zx_console_scroll_y)
        sub     #ZX_FONT_HEIGHT
        ld      b,a
        call    .zx_console_row_address
        ex      de,hl
        ld      bc,#32
        ldir
        ld      a,(_zx_console_scroll_y)
        inc     a
        ld      (_zx_console_scroll_y),a
        jr      .zx_console_scroll_loop

.zx_console_clear_tail:
        ld      a,#180
        ld      (_zx_console_scroll_y),a
.zx_console_clear_loop:
        ld      a,(_zx_console_scroll_y)
        cp      #192
        ret     z
        ld      b,a
        call    .zx_console_row_address
        ld      b,#32
        xor     a
.zx_console_clear_bytes:
        ld      (hl),a
        inc     hl
        djnz    .zx_console_clear_bytes
        ld      a,(_zx_console_scroll_y)
        inc     a
        ld      (_zx_console_scroll_y),a
        jr      .zx_console_clear_loop

;; YOS tv_rowaddr: B = pixel Y, returns HL = Spectrum bitmap row.
.zx_console_row_address:
        ld      a,b
        and     #0x07
        or      #0x40
        ld      h,a
        ld      a,b
        rrca
        rrca
        rrca
        and     #0x18
        or      h
        ld      h,a
        ld      a,b
        rla
        rla
        and     #0xe0
        ld      l,a
        ret

.zx_console_masks:
        .db     0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc

        .area   _BSS
_zx_console_x:
        .ds     1
_zx_console_row:
        .ds     1
_zx_console_glyph_width:
        .ds     1
_zx_console_advance_width:
        .ds     1
_zx_console_glyph_ptr:
        .ds     2
_zx_console_mask:
        .ds     1
_zx_console_bits:
        .ds     1
_zx_console_shift:
        .ds     1
_zx_console_byte_x:
        .ds     1
_zx_console_draw_y:
        .ds     1
_zx_console_rows_left:
        .ds     1
_zx_console_scroll_y:
        .ds     1
