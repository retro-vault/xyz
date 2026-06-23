        ; 16-bit integer to float stub for the merged xcc runtime.
        ; Returns 0.0f until a real soft-float conversion helper is
        ; linked.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fitosf
        .area   _CODE
        .globl  __fitosf

        ; __fitosf
        ; inputs: caller pushes one 16-bit integer argument.
        ; outputs: DE:HL = 0.0f.
        ; clobbers: DE, HL.

__fitosf:
        ld      hl, #0
        ld      de, #0
        ret
