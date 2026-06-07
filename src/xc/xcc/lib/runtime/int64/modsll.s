        ; signed 64-bit modulo (C11: sign of remainder = sign of dividend)
        ;
        ; Same algorithm as divsll but returns remainder instead of quotient.
        ;
        ; Frame layout (17 bytes):
        ;   ix-8..ix-1:  abs(a) / quotient (discarded)
        ;   ix-16..ix-9: remainder
        ;   ix-17:       sign_x (sign of original a, for remainder sign)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module modsll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __modsll
        .globl  __smod64

        ; __modsll / __smod64
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; outputs: DE:HL:DE':HL' = signed remainder (sign = sign of a)
        ; clobbers: af, bc, de, hl, ix, de', hl'

__smod64:
__modsll:
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      b, h
        ld      c, l

        ld      hl, #-17
        add     hl, sp
        ld      sp, hl

        ld      h, b
        ld      l, c

        ; sign_x = sign of a (bit7 of H', which is bit7 of HL' high byte)
        exx
        ld      a, h            ; H' = msb of a
        exx
        and     #0x80
        rlca
        ld      -17(ix), a      ; 0 if positive, 1 if negative

        ; abs(a)
        exx
        bit     7, h
        exx
        jr      z, .modsll_a_pos
        call    .neg64r
.modsll_a_pos:

        ld      -8(ix), e
        ld      -7(ix), d
        ld      -6(ix), l
        ld      -5(ix), h
        exx
        ld      -4(ix), e
        ld      -3(ix), d
        ld      -2(ix), l
        ld      -1(ix), h
        exx

        ; abs(b) in-place on stack
        bit     7, 11(ix)
        jr      z, .modsll_b_pos
        call    .negstackr
.modsll_b_pos:

        xor     a
        ld      -16(ix), a
        ld      -15(ix), a
        ld      -14(ix), a
        ld      -13(ix), a
        ld      -12(ix), a
        ld      -11(ix), a
        ld      -10(ix), a
        ld      -9(ix),  a

        ld      b, #64
.modsll_loop:
        sla     -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        rl      -2(ix)
        rl      -1(ix)

        rl      -16(ix)
        rl      -15(ix)
        rl      -14(ix)
        rl      -13(ix)
        rl      -12(ix)
        rl      -11(ix)
        rl      -10(ix)
        rl      -9(ix)

        or      a
        ld      a, -16(ix)
        sbc     a, 4(ix)
        ld      -16(ix), a
        ld      a, -15(ix)
        sbc     a, 5(ix)
        ld      -15(ix), a
        ld      a, -14(ix)
        sbc     a, 6(ix)
        ld      -14(ix), a
        ld      a, -13(ix)
        sbc     a, 7(ix)
        ld      -13(ix), a
        ld      a, -12(ix)
        sbc     a, 8(ix)
        ld      -12(ix), a
        ld      a, -11(ix)
        sbc     a, 9(ix)
        ld      -11(ix), a
        ld      a, -10(ix)
        sbc     a, 10(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        sbc     a, 11(ix)
        ld      -9(ix), a
        jp      nc, .modsll_keep

        or      a
        ld      a, -16(ix)
        adc     a, 4(ix)
        ld      -16(ix), a
        ld      a, -15(ix)
        adc     a, 5(ix)
        ld      -15(ix), a
        ld      a, -14(ix)
        adc     a, 6(ix)
        ld      -14(ix), a
        ld      a, -13(ix)
        adc     a, 7(ix)
        ld      -13(ix), a
        ld      a, -12(ix)
        adc     a, 8(ix)
        ld      -12(ix), a
        ld      a, -11(ix)
        adc     a, 9(ix)
        ld      -11(ix), a
        ld      a, -10(ix)
        adc     a, 10(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        adc     a, 11(ix)
        ld      -9(ix), a
        jp      .modsll_next

.modsll_keep:
        set     0, -8(ix)

.modsll_next:
        dec     b
        jp      nz, .modsll_loop

        ; Load remainder (ix-16..ix-9) into registers
        ld      e, -16(ix)
        ld      d, -15(ix)
        ld      l, -14(ix)
        ld      h, -13(ix)
        exx
        ld      e, -12(ix)
        ld      d, -11(ix)
        ld      l, -10(ix)
        ld      h, -9(ix)
        exx

        ; Negate remainder if original a was negative
        ld      a, -17(ix)
        or      a
        jr      z, .modsll_done
        call    .neg64r

.modsll_done:
        ld      sp, ix
        pop     ix
        ret

.neg64r:
        xor     a
        sub     a, e
        ld      e, a
        ld      a, #0
        sbc     a, d
        ld      d, a
        ld      a, #0
        sbc     a, l
        ld      l, a
        ld      a, #0
        sbc     a, h
        ld      h, a
        exx
        ld      a, #0
        sbc     a, e
        ld      e, a
        ld      a, #0
        sbc     a, d
        ld      d, a
        ld      a, #0
        sbc     a, l
        ld      l, a
        ld      a, #0
        sbc     a, h
        ld      h, a
        exx
        ret

.negstackr:
        xor     a
        sub     a, 4(ix)
        ld      4(ix), a
        ld      a, #0
        sbc     a, 5(ix)
        ld      5(ix), a
        ld      a, #0
        sbc     a, 6(ix)
        ld      6(ix), a
        ld      a, #0
        sbc     a, 7(ix)
        ld      7(ix), a
        ld      a, #0
        sbc     a, 8(ix)
        ld      8(ix), a
        ld      a, #0
        sbc     a, 9(ix)
        ld      9(ix), a
        ld      a, #0
        sbc     a, 10(ix)
        ld      10(ix), a
        ld      a, #0
        sbc     a, 11(ix)
        ld      11(ix), a
        ret
