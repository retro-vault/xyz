        ; 16-bit multiply entry point in the modern SDCC ABI.
        ;
        ; inputs:  HL = left, DE = right
        ; outputs: DE = low 16 bits of the product
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mul16_bridge
        .area   _CODE
        .globl  __mul16
        .globl  __mulint

__mul16:
        jp      __mulint
