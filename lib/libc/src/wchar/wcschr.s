        ; wcschr.s — first occurrence of a wide char (c==0 -> terminator).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcschr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcschr
        .area   _CODE
        ; HL = s, DE = c -> DE = pointer or 0
_wcschr::
wch_loop:
        ld      a,(hl)
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      c,a                     ; BC = *s
        cp      e
        jr      nz,wch_nomatch
        ld      a,b
        cp      d
        jr      nz,wch_nomatch
        dec     hl
        dec     hl
        ex      de,hl
        ret
wch_nomatch:
        ld      a,b
        or      c
        jr      nz,wch_loop
        ld      de,#0                   ; end of string, not found
        ret
