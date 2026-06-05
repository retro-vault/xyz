        ; 32-bit signed modulus entry point matching the modern SDCC ABI.
        ;
        ; inputs:  DE:HL = dividend (DE low, HL high)
        ;          stack = divisor bytes
        ; outputs: DE:HL = remainder
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module smod32_bridge
        .area   _CODE
        .globl  __smod32
        .globl  __modslong

__smod32:
        jp      __modslong
