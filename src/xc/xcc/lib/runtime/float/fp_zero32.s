        ; 32-bit floating zero helper
        ;
        ; shared by float arithmetic and conversions that need a zeroed
        ; DE:HL result
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fp_zero32
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fp_zero32

        ; __fp_zero32
        ; inputs: n/a
        ; outputs: de:hl = 0x00000000
        ; clobbers: af, de, hl
__fp_zero32:
        xor     a
        ld      h, a
        ld      l, a
        ld      d, a
        ld      e, a
        ret
