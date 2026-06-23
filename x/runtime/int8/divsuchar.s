        ; divsuchar.s
        ; signed dividend, unsigned divisor 8-bit division shim
        ;
        ; builds hl from dividend and e from divisor, then tail-calls
        ; __div_signexte
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module divsuchar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __divsuchar_rrx_s
        .globl  __divsuchar_rrf_s
        .globl  __divsuchar
        .globl  __div_signexte

        ; __divsuchar
        ; inputs:  a = signed dividend, l = unsigned divisor
        ; outputs: de = quotient, hl = remainder
        ; clobbers: a, d, e, h, l, f
__divsuchar_rrx_s::
__divsuchar_rrf_s::
__divsuchar:
        ld      e, l
        ld      l, a
        rlca
        sbc     a, a
        ld      h, a
        jp      __div_signexte
