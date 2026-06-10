        ; wcscpy.s — copy wide string including terminator.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcscpy
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcscpy
        .area   _CODE
        ;; _wcscpy
        ;; Copy 16-bit elements until the terminating wide NUL has been copied.
_wcscpy::
        push    hl
wcp_loop:
        ld      a,(de)
        ld      (hl),a
        ld      c,a
        inc     de
        inc     hl
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        or      c
        jr      nz,wcp_loop
        pop     de
        ret
