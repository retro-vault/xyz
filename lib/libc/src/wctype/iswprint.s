        ; iswprint.s
        ;
        ; libc iswprint() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isprint().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswprint
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswprint
        .globl  _isprint
        .area   _CODE
        ;; _iswprint
        ;; Only byte-sized execution characters can be classified as printable.
_iswprint::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isprint
isw_false:
        ld      de,#0
        ret
