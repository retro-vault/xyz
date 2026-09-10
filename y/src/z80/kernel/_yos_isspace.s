        ; ASCII whitespace classifier for the YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_isspace
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_isspace

        .area   _CODE

        ; __yos_isspace, internal service-table adapter
        ; input: hl = character value
        ; output: de = one for ASCII whitespace, otherwise zero
        ; clobbers: af, de; preserves bc, hl, ix and iy
__yos_isspace::
        ld      a, l
        cp      #' '
        jr      z, .space_true
        sub     #9
        cp      #5
        ld      de, #0
        ret     nc
.space_true:
        ld      de, #1
        ret
