        ; ABI bridge for 16-bit signed modulus.
        ; Does NOT use __modsint (broken for opposite-sign operands).
        ; Instead uses __divuint on absolute values, then sign-corrects.
        ; Sign of remainder = sign of dividend (C11 semantics).
        ;
        ; __divuint clobbers af, bc, de, hl, f; so sign(dividend) is
        ; saved on the stack via push af before the call.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module smod16_bridge
        .area   _CODE
        .globl  __smod16
        .globl  __divuint

        ; __smod16
        ; inputs: 4(ix)..5(ix) = dividend, 6(ix)..7(ix) = divisor.
        ; outputs: HL = remainder (signed, sign matches dividend).
        ; clobbers: af, bc, de, hl, ix.

__smod16:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      l, 4(ix)
        ld      h, 5(ix)                ; HL = dividend
        ld      e, 6(ix)
        ld      d, 7(ix)                ; DE = divisor

        ; save sign(dividend) on stack before abs operations clobber a
        ld      a, h
        rla                             ; C = sign(dividend)
        push    af                      ; save carry (sign bit)

        ; abs(dividend) if negative
        jr      nc, .smod16_d_pos
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.smod16_d_pos:

        ; abs(divisor) if negative
        bit     7, d
        jr      z, .smod16_v_pos
        xor     a
        sub     a, e
        ld      e, a
        sbc     a, a
        sub     a, d
        ld      d, a
.smod16_v_pos:

        call    __divuint               ; HL = remainder (unsigned), DE = quotient

        ; restore sign of dividend from stack
        pop     af
        jr      nc, .smod16_done        ; dividend positive: remainder correct
        ; negate HL (make remainder negative)
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.smod16_done:
        pop     ix
        ret
