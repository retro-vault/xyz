        ; 32-bit signed division entry point matching the modern SDCC ABI.
        ;
        ; inputs:  DE:HL = dividend (DE low, HL high)
        ;          stack = divisor bytes
        ; outputs: DE:HL = quotient
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sdiv32_bridge
        .area   _CODE
        .globl  __sdiv32
        .globl  __divslong

__sdiv32:
        jp      __divslong
