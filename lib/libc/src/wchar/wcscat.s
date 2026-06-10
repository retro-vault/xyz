        ; wcscat.s — append wide string.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcscat
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcscat
        .area   _CODE
        ;; _wcscat
        ;; Walk dst to its terminating wchar_t, then copy the source string
        ;; including its terminator so the result remains properly terminated.
_wcscat::
        push    hl
wca_end:
        ld      a,(hl)
        ld      c,a
        inc     hl
        ld      a,(hl)
        inc     hl
        or      c
        jr      nz,wca_end
        dec     hl
        dec     hl                      ; Rewind from the probe step to the terminator slot.
wca_copy:
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
        jr      nz,wca_copy
        pop     de
        ret
