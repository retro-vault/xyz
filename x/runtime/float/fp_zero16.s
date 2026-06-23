        ; 16-bit floating zero helper
        ;
        ; kept as a separate object so 32-bit float users do not pull it
        ; in unnecessarily
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fp_zero16
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_zero16

        ; __fp_zero16
        ; inputs: n/a
        ; outputs: hl = 0x0000
        ; clobbers: af, hl
__fp_zero16:
        xor     a
        ld      h, a
        ld      l, a
        ret
