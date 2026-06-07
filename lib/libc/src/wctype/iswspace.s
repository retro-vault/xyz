        ; iswspace.s
        ;
        ; libc iswspace() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isspace().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswspace
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswspace
        .globl  _isspace
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswspace::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isspace
isw_false:
        ld      de,#0
        ret
