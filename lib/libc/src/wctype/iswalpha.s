        ; iswalpha.s
        ;
        ; libc iswalpha() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isalpha().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswalpha
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswalpha
        .globl  _isalpha
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswalpha::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isalpha
isw_false:
        ld      de,#0
        ret
