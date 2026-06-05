        ; 32-bit unsigned modulus entry point matching the modern SDCC ABI.
        ;
        ; inputs:  DE:HL = dividend (DE low, HL high)
        ;          stack = divisor bytes
        ; outputs: DE:HL = remainder
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mod32_bridge
        .area   _CODE
        .globl  __mod32
        .globl  __modulong

__mod32:
        jp      __modulong
