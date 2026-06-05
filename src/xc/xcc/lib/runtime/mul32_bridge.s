        ; 32-bit multiply entry point matching the modern SDCC ABI.
        ;
        ; inputs:  DE:HL = left (DE low, HL high)
        ;          stack = right operand bytes
        ; outputs: DE:HL = low 32 bits of the product
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mul32_bridge
        .area   _CODE
        .globl  __mul32
        .globl  __mullong

__mul32:
        jp      __mullong
