        ; iswlower.s
        ;
        ; libc iswlower() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow islower().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswlower
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswlower
        .globl  _islower
        .area   _CODE
        ;; _iswlower
        ;; Lowercase classification is only meaningful for byte-sized execution
        ;; characters in this libc.
_iswlower::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _islower
isw_false:
        ld      de,#0
        ret
