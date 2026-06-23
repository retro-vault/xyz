        ; wctrans.s
        ;
        ; libc wctrans() for the xcc Z80 libc.  "tolower" -> 1, "toupper" -> 2,
        ; everything else -> 0.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module wctrans
        .optsdcc -mz80 sdcccall(1)
        .globl  _wctrans
        .globl  _strcmp
        .area   _CODE

        ;; _wctrans
        ;; Map the textual transformation name onto the compact descriptor used by
        ;; towctrans. Descriptor 0 is reserved for "unknown".
_wctrans::
        ld      a,h
        or      l
        jr      z,wtr_none
        push    hl
        ld      de,#__wtr_lower
        call    _strcmp
        ld      a,d
        or      e
        pop     hl
        jr      z,wtr_l
        push    hl
        ld      de,#__wtr_upper
        call    _strcmp
        ld      a,d
        or      e
        pop     hl
        jr      z,wtr_u
wtr_none:
        ld      de,#0
        ret
wtr_l:
        ld      de,#1
        ret
wtr_u:
        ld      de,#2
        ret
__wtr_lower: .asciz "tolower"
__wtr_upper: .asciz "toupper"
