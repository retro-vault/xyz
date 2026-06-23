        ; signed 8-bit modulus front-end helper
        ;
        ; divides through the shared signed 8-bit divide front-end, then
        ; normalizes the remainder sign to match the dividend
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2009-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module modschar
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE                   ; code segment

        .globl  __modschar_rrx_s
        .globl  __modschar_rrf_s
        .globl  __modschar
        .globl  __div8
        .globl  __get_remainder

        ; __modschar
        ; inputs:  a = dividend (signed 8-bit), l = divisor (signed 8-bit)
        ; outputs: de = dividend % divisor (signed 16-bit remainder)
        ; clobbers: a, d, e, h, l, f
__modschar_rrx_s::
__modschar_rrf_s::
__modschar:
        ld      e, l
        ld      l, a
        call    __div8
        call    __get_remainder
        ex      de, hl
        ret
