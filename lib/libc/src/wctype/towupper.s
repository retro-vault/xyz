        ; towupper.s
        ;
        ; libc towupper() for the xcc Z80 libc.  Wide characters above UCHAR_MAX are
        ; returned unchanged; otherwise the narrow toupper() is applied and the
        ; result narrowed back to one byte.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module towupper
        .optsdcc -mz80 sdcccall(1)
        .globl  _towupper
        .globl  _toupper
        .area   _CODE
        ;; _towupper
        ;; Only byte-sized wide characters participate in the execution charset.
        ;; Larger wchar_t values are returned unchanged instead of being narrowed.
_towupper::
        ld      a,h
        or      a
        jr      nz,tow_id
        call    _toupper
        ld      d,#0                    ; Re-promote the narrow result back to wint_t.
        ret
tow_id:
        ex      de,hl                   ; Return wc unchanged when it is not byte-sized.
        ret
