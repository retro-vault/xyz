        ; divuschar.s
        ; unsigned dividend, signed divisor 8-bit division shim
        ;
        ; builds signed 16-bit operands, then tail-calls __divsint
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module divuschar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __divuschar_rrx_s
        .globl  __divuschar_rrf_s
        .globl  __divuschar
        .globl  __div_signexte

        ; __divuschar
        ; inputs:  a = unsigned dividend, l = signed divisor
        ; outputs: de = quotient, hl = remainder
        ; clobbers: a, d, e, h, l, f
__divuschar_rrx_s::
__divuschar_rrf_s::
__divuschar:
        ld      e, l
        ld      l, a
        ld      h, #0
        jp      __div_signexte
