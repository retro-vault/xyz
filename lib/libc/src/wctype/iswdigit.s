        ; iswdigit.s
        ;
        ; libc iswdigit() for the xcc Z80 libc.  Single-byte execution charset, so a
        ; wide character above UCHAR_MAX is never of this class; otherwise it
        ; defers to the narrow isdigit().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswdigit
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswdigit
        .globl  _isdigit
        .area   _CODE
        ;; _iswdigit
        ;; Decimal-digit classification is delegated to the narrow ASCII helper
        ;; once the wchar_t value is known to fit in one byte.
_iswdigit::
        ld      a,h
        or      a
        jr      nz,isw_false
        jp      _isdigit
isw_false:
        ld      de,#0
        ret
