        ; div.s
        ;
        ; libc div implementation for the xcc Z80 libc.
        ; Computes the signed 16-bit quotient and remainder in one call.
        ; The div_t result {int quot; int rem;} fits in 4 bytes and is
        ; returned in DE:HL (DE = quot, HL = rem), matching the xcc
        ; small-struct return convention.
        ;
        ; Delegates to the shared signed-divide core: __divsint produces the
        ; quotient (DE) and an unsigned remainder (HL), and __get_remainder
        ; corrects the remainder sign to match the dividend (C truncation),
        ; preserving the quotient in DE.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module div
        .optsdcc -mz80 sdcccall(1)


        .globl  _div
        .globl  __divsint
        .globl  __get_remainder

        .area   _CODE

        ; _div
        ; inputs:  HL = numerator, DE = denominator (both signed int)
        ; outputs: DE = quotient, HL = remainder (sign matches numerator)
        ; clobbers: AF, BC
_div::
        call    __divsint               ; DE = quot, HL = unsigned rem
        jp      __get_remainder         ; HL = signed rem, DE preserved
