        ; iswpunct.s
        ;
        ; libc iswpunct() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow ispunct().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswpunct
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswpunct
        .globl  _ispunct
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswpunct::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _ispunct
isw_false:
        ld      de,#0
        ret
