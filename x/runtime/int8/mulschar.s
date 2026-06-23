        ; mulschar.s
        ; signed lhs, signed rhs 8x8->16 multiply shim
        ;
        ; prepares operands in hl and de, then tail-calls __mul16
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2017-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module mulschar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __mulschar_rrx_s
        .globl  __mulschar_rrf_s
        .globl  __mulschar
        .globl  __mul16

        ; __mulschar
        ; inputs:  a = signed lhs, l = signed rhs
        ; outputs: de = 16-bit product (via __mul16)
        ; clobbers: a, d, e, h, l, f; plus any clobbers from __mul16
__mulschar_rrx_s::
__mulschar_rrf_s::
__mulschar:
        ld      e, l
        ld      l, a
        rlca
        sbc     a, a
        ld      h, a

        ld      a, e
        rlca
        sbc     a, a
        ld      d, a

        jp      __mul16
