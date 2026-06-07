        ; modsuchar.s
        ; signed dividend, unsigned divisor 8-bit modulus shim
        ;
        ; divides through __div_signexte, then tail-jumps to
        ; __get_remainder
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module modsuchar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __modsuchar_rrx_s
        .globl  __modsuchar_rrf_s
        .globl  __modsuchar
        .globl  __div_signexte
        .globl  __get_remainder

        ; __modsuchar
        ; inputs:  a = signed dividend, l = unsigned divisor
        ; outputs: de = remainder
        ; clobbers: a, d, e, h, l, f; plus divide-core clobbers
__modsuchar_rrx_s::
__modsuchar_rrf_s::
__modsuchar:
        ld      e, l
        ld      l, a
        rlca
        sbc     a, a
        ld      h, a
        call    __div_signexte
        call    __get_remainder
        ex      de, hl
        ret
