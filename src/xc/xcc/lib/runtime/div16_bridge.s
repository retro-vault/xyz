        ; 16-bit unsigned division entry point in the modern SDCC ABI.
        ;
        ; inputs:  HL = dividend, DE = divisor
        ; outputs: DE = quotient, HL = remainder
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module div16_bridge
        .area   _CODE
        .globl  __div16
        .globl  __divuint

__div16:
        jp      __divuint
