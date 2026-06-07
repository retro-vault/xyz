        ; iswalnum.s
        ;
        ; libc iswalnum() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isalnum().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswalnum
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswalnum
        .globl  _isalnum
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswalnum::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isalnum
isw_false:
        ld      de,#0
        ret
