        ; wcsnlen.s — bounded wide string length.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsnlen
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsnlen
        .area   _CODE
        ;; _wcsnlen
        ;; Count wide elements until either maxlen runs out or the wide NUL
        ;; terminator is encountered.
_wcsnlen::
        ld      b,d
        ld      c,e                     ; BC = maxlen
        ld      de,#0                   ; length
wnl_loop:
        ld      a,b
        or      c
        jr      z,wnl_done
        ld      a,(hl)
        inc     hl
        or      (hl)
        inc     hl
        jr      z,wnl_done
        inc     de
        dec     bc
        jr      wnl_loop
wnl_done:
        ret
