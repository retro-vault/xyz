        ; signed 16-bit modulus helper
        ; uses the signed 16-bit divide core and normalizes remainder sign
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2009-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module modsigned
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE                   ; code segment

        .globl  __modsint_rrx_s
        .globl  __modsint_rrf_s
        .globl  __modsint
        .globl  __divsint
        .globl  __get_remainder

        ; __modsint
        ; inputs:  hl = dividend (signed 16-bit), de = divisor (signed
        ; 16-bit)
        ; outputs: de = dividend % divisor (signed 16-bit remainder)
        ; clobbers: a, b, c, d, e, h, l, f; plus any clobbers from
        ; __divsint /
        ;           __get_remainder
        ; notes: __divsint produces quotient/remainder; __get_remainder
        ;        returns properly signed remainder in hl, which we move
        ;        into the public DE return register
__modsint_rrx_s::
__modsint_rrf_s::
__modsint:
        call    __divsint               ; signed divide 16-bit
        call    __get_remainder
        ex      de, hl
        ret
