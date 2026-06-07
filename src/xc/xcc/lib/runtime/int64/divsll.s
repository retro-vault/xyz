        ; signed 64-bit divide
        ;
        ; Strategy: abs(a), abs(b_copy), unsigned divide, negate if signs differ.
        ;
        ; Frame layout (17 bytes):
        ;   ix-8..ix-1:  abs(a) / quotient
        ;   ix-16..ix-9: remainder
        ;   ix-17:       sign_q (0=positive, 1=negative)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module divsll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __divsll
        .globl  __sdiv64

        ; __divsll / __sdiv64
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; outputs: DE:HL:DE':HL' = signed quotient (truncated toward zero)
        ; clobbers: af, bc, de, hl, ix, de', hl'

__sdiv64:
__divsll:
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

        ; sign_q = sign(a) XOR sign(b)
        exx
        ld      a, h            ; H' = msb of a
        exx
        ld      b, a            ; save sign of a
        ld      a, 11(ix)       ; msb of b
        xor     b               ; XOR signs
        and     #0x80
        rlca                    ; sign bit to bit0
        ld      -17(ix), a      ; store sign_q (0 or 1)

        ; abs(a): negate if negative (bit7 of H' set)
        exx
        bit     7, h
        exx
        jr      z, .divsll_a_pos
        call    .neg64
.divsll_a_pos:

        ; Store abs(a) in frame
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

        ; abs(b): negate in-place on stack if negative (bit7 of ix+11)
        bit     7, 11(ix)
        jr      z, .divsll_b_pos
        call    .negstack
.divsll_b_pos:

        ; Zero remainder
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
.divsll_loop:
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
        jp      nc, .divsll_keep

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
        jp      .divsll_next

.divsll_keep:
        set     0, -8(ix)

.divsll_next:
        dec     b
        jp      nz, .divsll_loop

        ; Load quotient (ix-8..ix-1) into registers
        ld      e, -8(ix)
        ld      d, -7(ix)
        ld      l, -6(ix)
        ld      h, -5(ix)
        exx
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      l, -2(ix)
        ld      h, -1(ix)
        exx

        ; Negate if sign_q = 1
        ld      a, -17(ix)
        or      a
        jr      z, .divsll_done
        call    .neg64

.divsll_done:
        ld      sp, ix
        pop     ix
        ret

        ; Negate DE:HL:DE':HL' (two's complement)
.neg64:
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

        ; Negate the 8-byte b on stack (ix+4..ix+11) in-place
.negstack:
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
