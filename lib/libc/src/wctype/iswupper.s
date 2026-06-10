        ; iswupper.s
        ;
        ; libc iswupper() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isupper().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswupper
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswupper
        .globl  _isupper
        .area   _CODE
        ;; _iswupper
        ;; Uppercase classification is only defined for byte-sized execution
        ;; characters in this libc.
_iswupper::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isupper
isw_false:
        ld      de,#0
        ret
