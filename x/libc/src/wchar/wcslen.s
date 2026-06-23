        ; wcslen.s — wide string length.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcslen
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcslen
        .area   _CODE
        ;; _wcslen
        ;; Count 16-bit elements until the wide NUL terminator is reached.
_wcslen::
        ld      de,#0
wl_loop:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        inc     hl
        or      c
        ret     z
        inc     de
        jr      wl_loop
