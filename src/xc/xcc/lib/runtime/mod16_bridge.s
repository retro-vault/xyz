        ; 16-bit unsigned modulus entry point in the modern SDCC ABI.
        ;
        ; inputs:  HL = dividend, DE = divisor
        ; outputs: DE = remainder
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mod16_bridge
        .area   _CODE
        .globl  __mod16
        .globl  __moduint

__mod16:
        jp      __moduint
