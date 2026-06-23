        ; wcsspn.s — leading run length of chars in accept.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsspn
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsspn
        .globl  __wchar_is_delim
        .area   _CODE
        ; HL = s, DE = accept -> DE = count
_wcsspn::
        push    hl
wcss_loop:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        ld      b,a
        dec     hl
        ld      a,b
        or      c
        jr      z,wcss_done
        call    __wchar_is_delim        ; Z if *s in accept
        jr      nz,wcss_done            ; not in accept -> stop
        inc     hl
        inc     hl
        jr      wcss_loop
wcss_done:
        ex      de,hl
        pop     hl
        ld      a,e
        sub     l
        ld      e,a
        ld      a,d
        sbc     h
        ld      d,a
        srl     d
        rr      e
        ret
