        ; Expand the compact built-in font into fixed common RAM at boot.
        ;
        ; GPL-2.0 License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _gpx_font_expand
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_font_init
        .globl  _gpx_font_envy
        .globl  __gpx_font_widths
        .globl  __gpx_font_packed
        .globl  __os_malloc

        .area   _BSS
_gpx_font_envy::
        .ds     2

        .area   _CODE

        ; The source face has a fixed eight-byte header, 96 offsets, a
        ; five-byte space record and 95 uniform 13-byte glyph records.
        ; ROM stores 21 exceptional widths plus packed raster bits.
__gpx_font_init::
        ld      hl,#1440
        call    __os_malloc             ; no current process: OS-owned block
        ld      (_gpx_font_envy),de
        ld      a,d
        or      e
        ret     z
        ld      hl,#.header             ; HL = packed header, DE = destination
        ld      bc,#8
        ldir
        ex      de,hl                   ; HL = expanded destination after header
        ld      de,#0x00c8
        ld      b,#96
.offset:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      a,b
        cp      #96
        ld      a,#13
        jr      nz,.advance
        ld      a,#5
.advance:
        add     a,e
        ld      e,a
        jr      nc,.offset_next
        inc     d
.offset_next:
        djnz    .offset
        xor     a                       ; space: sig,width,size = zero
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),#8
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      de,#__gpx_font_packed
        exx
        ld      hl,#__gpx_font_widths  ; width,next-exception-delta pairs
        ld      e,#1                   ; first exception is '!'
        ld      b,#95                  ; alternate B = glyph count
        exx
.glyph:
        ld      (hl),#0                 ; bitmap signature
        inc     hl
        exx
        ld      c,#5                    ; nearly every glyph uses width five
        dec     e
        jr      nz,.have_width
        ld      c,(hl)
        inc     hl
        ld      e,(hl)
        inc     hl
.have_width:
        ld      d,#8                    ; alternate D = rows remaining
        ld      a,c
        exx
        ld      (hl),a
        inc     hl
        ld      (hl),#8                 ; height
        inc     hl
        ld      (hl),#8                 ; payload size
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      c,#0x80                 ; packed-source bit mask
.row:
        ld      (hl),#0
        exx
        ld      a,c                     ; width
        exx
        ld      b,a
.pixel:
        sla     (hl)
        ld      a,(de)
        and     c
        jr      z,.zero
        inc     (hl)
.zero:
        srl     c
        jr      nz,.same_byte
        inc     de
        ld      c,#0x80
.same_byte:
        djnz    .pixel
        exx
        ld      a,#8
        sub     c                       ; restore the source's left alignment
        exx
        jr      z,.aligned
        ld      b,a
.align:
        sla     (hl)
        djnz    .align
.aligned:
        inc     hl
        exx
        dec     d
        ld      a,d
        exx
        or      a
        jr      nz,.row
        exx
        djnz    .more_glyphs
        exx
        ret
.more_glyphs:
        exx
        jr      .glyph

.header:
        .db     1,0x20,0x7f,5,8,8,1,1
