        ; Float to 16-bit integer stub for the merged xcc runtime.
        ; Returns zero until a real soft-float conversion helper is
        ; linked.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fstoi
        .area   _CODE
        .globl  __fstoi

        ; __fstoi
        ; inputs: caller pushes one 32-bit float argument.
        ; outputs: HL = 0.
        ; clobbers: DE, HL.

__fstoi:
        ld      hl, #0
        ld      de, #0
        ret
