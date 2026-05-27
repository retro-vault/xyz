        ; signed/unsigned mixed modulus helpers (8-bit × 8-bit)
        ; handles combinations of signed/unsigned dividend and divisor
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module modmixed
        .optsdcc -mz80 sdcccall(1)

        .globl  __modsuchar_rrx_s
        .globl  __modsuchar_rrf_s
        .globl  __modsuchar             ; export symbols
        .globl  __moduschar_rrx_s
        .globl  __moduschar_rrf_s
        .globl  __moduschar
        .globl  __div16
        .globl  __div_signexte
        .globl  __get_remainder

        ; __modsuchar
        ; inputs:  a = signed dividend (8-bit), l = unsigned divisor
        ; (8-bit)
        ; outputs: hl = remainder (signed 16-bit, low byte holds result)
        ; clobbers: a, d, e, h, l, f; plus any clobbers from
        ; __div_signexte /
        ;           __get_remainder
        ; notes: build hl as signed-extended dividend, e = divisor,
        ;        call __div_signexte to divide, then normalize remainder
__modsuchar_rrx_s::
__modsuchar_rrf_s::
__modsuchar:
        ld      e, l
        ld      l, a                    ; l = dividend low
        ld      h, #0
        call    __div_signexte
        jp      __get_remainder

        ; __moduschar
        ; inputs:  a = unsigned dividend (8-bit), l = signed divisor
        ; (8-bit)
        ; outputs: hl = remainder (signed 16-bit, low byte holds result)
        ; clobbers: a, d, e, h, l, f; plus any clobbers from __div16 /
        ;           __get_remainder
        ; notes: build hl as sign-extended divisor, de = unsigned
        ; dividend,
        ;        perform signed divide, then normalize remainder
__moduschar_rrx_s::
__moduschar_rrf_s::
__moduschar:
        ld      e, l                    ; e = divisor (signed)
        ld      d, #0
        ld      l, a                    ; l = dividend low

        rlca
        sbc     a, a
        ld      h, a

        call    __div16                 ; signed 16-bit divide
        jp      __get_remainder
