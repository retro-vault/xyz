        ; iswcntrl.s
        ;
        ; libc iswcntrl() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow iscntrl().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswcntrl
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswcntrl
        .globl  _iscntrl
        .area   _CODE
        ;; _iswcntrl
        ;; The runtime has no Unicode control table, so only byte-sized values
        ;; can be classified through the narrow iscntrl implementation.
_iswcntrl::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _iscntrl
isw_false:
        ld      de,#0
        ret
