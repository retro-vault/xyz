        ; wcschr.s — first occurrence of a wide char (c==0 -> terminator).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcschr
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcschr
        .area   _CODE
        ;; _wcschr
        ;; Compare one 16-bit element at a time and return the address of the
        ;; first match. Like strchr, searching for 0 returns the terminator itself.
_wcschr::
wch_loop:
        ld      a,(hl)
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      c,a                     ; BC = current wchar_t
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
        ld      de,#0                   ; Fell off the terminator without a match.
        ret
