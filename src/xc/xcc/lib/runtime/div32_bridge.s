        ; 32-bit unsigned division entry point matching the modern SDCC ABI.
        ;
        ; inputs:  DE:HL = dividend (DE low, HL high)
        ;          stack = divisor bytes
        ; outputs: DE:HL = quotient
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module div32_bridge
        .area   _CODE
        .globl  __div32
        .globl  __divulong

__div32:
        jp      __divulong
