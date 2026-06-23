        ; 64-bit floating zero helper
        ; returns DE:HL:DE':HL' = 0.0 (all bytes zero)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module db_zero
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __db_zero

        ; __db_zero
        ; inputs: none
        ; outputs: DE:HL:DE':HL' = 0
        ; clobbers: af, de, hl, de', hl'
__db_zero:
        xor     a
        ld      d, a
        ld      e, a
        ld      h, a
        ld      l, a
        exx
        ld      d, a
        ld      e, a
        ld      h, a
        ld      l, a
        exx
        ret
