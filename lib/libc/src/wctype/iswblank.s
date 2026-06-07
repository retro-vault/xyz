        ; iswblank.s
        ;
        ; libc iswblank() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isblank().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswblank
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswblank
        .globl  _isblank
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswblank::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isblank
isw_false:
        ld      de,#0
        ret
