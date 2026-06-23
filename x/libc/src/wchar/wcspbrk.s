        ; wcspbrk.s — first char that is in accept (or NULL).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcspbrk
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcspbrk
        .globl  __wchar_is_delim
        .area   _CODE
        ; HL = s, DE = accept -> DE = pointer or 0
_wcspbrk::
wcpb_loop:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl
        ld      a,b
        or      c
        jr      z,wcpb_nf
        call    __wchar_is_delim        ; Z if *s in accept
        jr      z,wcpb_found
        inc     hl
        inc     hl
        jr      wcpb_loop
wcpb_found:
        ex      de,hl
        ret
wcpb_nf:
        ld      de,#0
        ret
