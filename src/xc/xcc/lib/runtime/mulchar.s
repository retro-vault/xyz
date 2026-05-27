        ; multiplication shims for signed/unsigned 8×8→16 bit
        ; prepares operands in bc and de, then tail-calls __mul16
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2017-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module mulchar
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE                   ; code segment

        .globl  __mulsuchar_rrx_s
        .globl  __mulsuchar_rrf_s
        .globl  __mulsuchar             ; export symbols
        .globl  __muluschar_rrx_s
        .globl  __muluschar_rrf_s
        .globl  __muluschar
        .globl  __mulschar_rrx_s
        .globl  __mulschar_rrf_s
        .globl  __mulschar
        .globl  __mul16                 ; imported

        ; __muluschar
        ; inputs:  a = signed lhs, l = unsigned rhs
        ; outputs: de = 16-bit product (via __mul16)
        ; clobbers: a, b, c, d, e, f; plus any clobbers from __mul16
        ; notes: builds bc signed (from a), de unsigned (from l)
__muluschar_rrx_s::
__muluschar_rrf_s::
__muluschar:
        ld      c, a                    ; c = lhs (signed)
        ld      a, c
        rlca
        sbc     a, a
        ld      b, a
        ld      e, l                    ; e = rhs (unsigned)
        ld      d, #0
        jp      __mul16

        ; __mulsuchar
        ; inputs:  a = unsigned lhs, l = signed rhs
        ; outputs: de = 16-bit product (via __mul16)
        ; clobbers: a, b, c, d, e, f; plus any clobbers from __mul16
        ; notes: builds bc unsigned (from a), de signed (from l)
__mulsuchar_rrx_s::
__mulsuchar_rrf_s::
__mulsuchar:
        ld      c, a                    ; c = lhs (unsigned)
        ld      b, #0
        ld      e, l                    ; e = rhs (signed)
        jr      .signext_e

        ; __mulschar
        ; inputs:  a = signed lhs, l = signed rhs
        ; outputs: de = 16-bit product (via __mul16)
        ; clobbers: a, b, c, d, e, f; plus any clobbers from __mul16
        ; notes: sign-extends both operands into bc and de
__mulschar_rrx_s::
__mulschar_rrf_s::
__mulschar:
        ld      e, l                    ; e = rhs (signed)
        ld      c, a                    ; c = lhs (signed)

        ; sign-extend c into b
        ld      a, c
        rlca
        sbc     a, a                    ; a = 00/ff
        ld      b, a

.signext_e:
        ; sign-extend e into d
        ld      a, e
        rlca
        sbc     a, a                    ; a = 00/ff
        ld      d, a

        ; tail-call: (bc) × (de) -> de (low 16 bits)
        jp      __mul16
