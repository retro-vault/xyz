        ; moduschar.s
        ; unsigned dividend, signed divisor 8-bit modulus shim
        ;
        ; divides through __divsint, then tail-jumps to __get_remainder
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module moduschar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __moduschar_rrx_s
        .globl  __moduschar_rrf_s
        .globl  __moduschar
        .globl  __div_signexte
        .globl  __get_remainder

        ; __moduschar
        ; inputs:  a = unsigned dividend, l = signed divisor
        ; outputs: de = remainder
        ; clobbers: a, d, e, h, l, f; plus divide-core clobbers
__moduschar_rrx_s::
__moduschar_rrf_s::
__moduschar:
        ld      e, l
        ld      l, a
        ld      h, #0
        call    __div_signexte
        call    __get_remainder
        ex      de, hl
        ret
