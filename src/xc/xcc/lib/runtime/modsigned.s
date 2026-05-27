        ; signed modulus helpers for 8-bit and 16-bit integers
        ; uses signed divide helpers and normalizes the remainder sign
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2009-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module modsigned
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE                   ; code segment

        .globl  __modschar_rrx_s
        .globl  __modschar_rrf_s
        .globl  __modschar              ; export symbols
        .globl  __modsint_rrx_s
        .globl  __modsint_rrf_s
        .globl  __modsint
        .globl  __div8
        .globl  __div16
        .globl  __get_remainder

        ; __modschar
        ; inputs:  a = dividend (signed 8-bit), l = divisor (signed
        ; 8-bit)
        ; outputs: l = dividend % divisor (signed 8-bit remainder)
        ; clobbers: a, d, e, h, l, f; plus any clobbers from __div8 /
        ;           __get_remainder
        ; notes: arrange params (l<-a, e<-orig l), call __div8, then
        ;        tail-jump to __get_remainder which adjusts remainder
        ; sign
__modschar_rrx_s::
__modschar_rrf_s::
__modschar:
        ld      e, l                    ; e = divisor (orig l)
        ld      l, a
        call    __div8                  ; signed divide 8-bit
        jp      __get_remainder

        ; __modsint
        ; inputs:  hl = dividend (signed 16-bit), de = divisor (signed
        ; 16-bit)
        ; outputs: hl = dividend % divisor (signed 16-bit remainder)
        ; clobbers: a, b, c, d, e, h, l, f; plus any clobbers from
        ; __div16 /
        ;           __get_remainder
        ; notes: __div16 produces quotient/remainder; __get_remainder
        ;        returns properly signed remainder in hl
__modsint_rrx_s::
__modsint_rrf_s::
__modsint:
        call    __div16                 ; signed divide 16-bit
        jp      __get_remainder
