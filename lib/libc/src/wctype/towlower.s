        ; towlower.s
        ;
        ; libc towlower() for the xcc Z80 libc.  Wide characters above UCHAR_MAX are
        ; returned unchanged; otherwise the narrow tolower() is applied and the
        ; result narrowed back to one byte.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module towlower
        .optsdcc -mz80 sdcccall(1)
        .globl  _towlower
        .globl  _tolower
        .area   _CODE
        ; HL = wc -> DE = mapped wc
_towlower::
        ld      a,h
        or      a
        jr      nz,tow_id
        call    _tolower
        ld      d,#0                    ; (unsigned char) result
        ret
tow_id:
        ex      de,hl                   ; return wc unchanged
        ret
