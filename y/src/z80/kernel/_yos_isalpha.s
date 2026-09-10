        ; ASCII alphabetic classifier for the YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_isalpha
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_isalpha

        .area   _CODE

        ; __yos_isalpha, internal service-table adapter
        ; input: hl = character value
        ; output: de = one for ASCII alphabetic, otherwise zero
        ; clobbers: af, de; preserves bc, hl, ix and iy
__yos_isalpha::
        ld      a, l
        and     #0xdf
        sub     #'A'
        cp      #26
        ld      de, #0
        ret     nc
        inc     e
        ret
