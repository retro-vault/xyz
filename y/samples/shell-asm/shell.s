        ; Minimal relocatable assembly shell for YOS ABI 1.
        ;
        ; It resolves the single yos_t interface, draws Hello World! in the
        ; centre of the screen without allocating memory, and loops forever.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module shell_asm
        .optsdcc -mz80 sdcccall(1)
        .include "yos.inc"
        .globl  _entry

        .area   _CODE
_entry::
        ld      hl,#.yos_name
        rst     0x18
        ld      a,d
        or      e
        jr      z,.idle
        push    de
        pop     ix                      ; IX = yos_t

        ld      bc,#YOS_OFFSET_GPX_GET_SYSTEM_FONT
        call    .function
        call    .invoke
        ld      a,d
        or      e
        jr      z,.idle
        push    de                      ; font

        ld      bc,#YOS_OFFSET_GPX_CLEAR_SCREEN
        call    .function
        call    .invoke

        ld      bc,#YOS_OFFSET_GPX_MEASURE_TEXT
        call    .function
        pop     de
        push    de                      ; keep font for drawing
        ld      hl,#.message
        call    .invoke                 ; DE = text width

        ld      hl,#256
        or      a
        sbc     hl,de
        srl     h
        rr      l
        push    hl                      ; x

        pop     de                      ; x
        pop     bc                      ; font
        push    bc                      ; font
        push    de                      ; x
        ld      h,b
        ld      l,c
        ld      de,#5
        add     hl,de
        ld      e,(hl)                  ; font->glyph_height
        ld      d,#0
        ld      hl,#192
        or      a
        sbc     hl,de
        srl     h
        rr      l                       ; HL = y

        push    hl                      ; preserve y while resolving function
        ld      bc,#YOS_OFFSET_GPX_DRAW_TEXT
        call    .function
        pop     hl                      ; y
        pop     de                      ; x
        pop     bc                      ; font

        push    hl
        pop     ix                      ; preserve y; yos_t no longer needed
        ld      hl,#0
        push    hl                      ; clip = NULL
        xor     a
        push    af
        inc     sp                      ; bmode = BM_CPY
        inc     a
        push    af
        inc     sp                      ; color = CO_FORE
        push    bc                      ; font
        ld      bc,#.message
        push    bc                      ; text
        push    ix                      ; y
        ld      hl,#0                   ; NULL context defaults to opaque
        call    .invoke                 ; callee removes 10 stack bytes

.idle:
        jr      .idle

        ; BC = yos_t byte offset; IY = resolved function.
.function:
        push    ix
        pop     hl
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        pop     iy
        ret

.invoke:
        jp      (iy)

.yos_name:
        .asciz  "yos"
.message:
        .asciz  "Hello World!"
