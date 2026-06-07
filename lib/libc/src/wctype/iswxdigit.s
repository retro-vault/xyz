        ; iswxdigit.s
        ;
        ; libc iswxdigit() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isxdigit().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswxdigit
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswxdigit
        .globl  _isxdigit
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswxdigit::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isxdigit
isw_false:
        ld      de,#0
        ret
