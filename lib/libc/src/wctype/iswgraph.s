        ; iswgraph.s
        ;
        ; libc iswgraph() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isgraph().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswgraph
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswgraph
        .globl  _isgraph
        .area   _CODE
        ; HL = wc -> DE = boolean
_iswgraph::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isgraph
isw_false:
        ld      de,#0
        ret
