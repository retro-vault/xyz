        ; mulsuchar.s
        ; unsigned lhs, signed rhs 8x8->16 multiply shim
        ;
        ; prepares operands in hl and de, then tail-calls __mul16
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2017-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module mulsuchar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE

        .globl  __mulsuchar_rrx_s
        .globl  __mulsuchar_rrf_s
        .globl  __mulsuchar
        .globl  __mul16

        ; __mulsuchar
        ; inputs:  a = unsigned lhs, l = signed rhs
        ; outputs: de = 16-bit product (via __mul16)
        ; clobbers: a, d, e, h, l, f; plus any clobbers from __mul16
__mulsuchar_rrx_s::
__mulsuchar_rrf_s::
__mulsuchar:
        ld      e, l
        ld      l, a
        ld      h, #0
        ld      a, e
        rlca
        sbc     a, a
        ld      d, a
        jp      __mul16
