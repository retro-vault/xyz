        ; ieee-754 double multiply:  result = a * b
        ; a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ;
        ; denormals treated as 0; no NaN/Inf; truncation.
        ;
        ; Frame (negative offsets from ix), 46 bytes:
        ;   -8 ..-1  : a bytes a0..a7
        ;   -22..-9  : acc[0..13]  (product accumulator, 14 bytes)
        ;   -36..-23 : mcand[0..13] (multiplicand = mant_b, 14 bytes)
        ;   -43..-37 : mplier[0..6] (multiplier = mant_a, 7 bytes)
        ;   -45..-44 : result exponent (lo at -45, hi at -44)
        ;   -46      : result sign
        ;
        ; significand: implicit 1 at bit52 = byte6 bit4.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbmul
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __dbmul
        .globl  ___dbmul
        .globl  __db_zero

__dbmul:
___dbmul::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-46
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
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

        ; exp_a
        ld      a, -1(ix)
        and     #0x7F
        ld      h, #0
        ld      l, a
        add     hl, hl
        add     hl, hl
        add     hl, hl
        add     hl, hl
        ld      a, -2(ix)
        rlca
        rlca
        rlca
        rlca
        and     #0x0F
        or      l
        ld      l, a            ; HL = exp_a
        ld      a, h
        or      l
        jp      nz, .a_nz
        jp      .ret_zero
.a_nz:
        push    hl
        ; exp_b
        ld      a, 11(ix)
        and     #0x7F
        ld      h, #0
        ld      l, a
        add     hl, hl
        add     hl, hl
        add     hl, hl
        add     hl, hl
        ld      a, 10(ix)
        rlca
        rlca
        rlca
        rlca
        and     #0x0F
        or      l
        ld      l, a            ; HL = exp_b
        ld      a, h
        or      l
        jp      nz, .b_nz
        pop     hl
        jp      .ret_zero
.b_nz:
        pop     de              ; DE = exp_a
        add     hl, de          ; HL = exp_a + exp_b
        ld      de, #1023
        or      a
        sbc     hl, de          ; HL = exp_a + exp_b - 1023
        ld      -44(ix), h
        ld      -45(ix), l
        ; result sign
        ld      a, -1(ix)
        xor     11(ix)
        and     #0x80
        ld      -46(ix), a

        ; build mplier = mant_a (7 bytes) at -43..-37 (m0..m6)
        ld      a, -8(ix)
        ld      -43(ix), a
        ld      a, -7(ix)
        ld      -42(ix), a
        ld      a, -6(ix)
        ld      -41(ix), a
        ld      a, -5(ix)
        ld      -40(ix), a
        ld      a, -4(ix)
        ld      -39(ix), a
        ld      a, -3(ix)
        ld      -38(ix), a
        ld      a, -2(ix)
        and     #0x0F
        or      #0x10
        ld      -37(ix), a

        ; build mcand = mant_b (14 bytes): low 7 = mant_b, high 7 = 0
        ld      a, 4(ix)
        ld      -36(ix), a
        ld      a, 5(ix)
        ld      -35(ix), a
        ld      a, 6(ix)
        ld      -34(ix), a
        ld      a, 7(ix)
        ld      -33(ix), a
        ld      a, 8(ix)
        ld      -32(ix), a
        ld      a, 9(ix)
        ld      -31(ix), a
        ld      a, 10(ix)
        and     #0x0F
        or      #0x10
        ld      -30(ix), a
        xor     a
        ld      -29(ix), a
        ld      -28(ix), a
        ld      -27(ix), a
        ld      -26(ix), a
        ld      -25(ix), a
        ld      -24(ix), a
        ld      -23(ix), a

        ; acc = 0 (14 bytes)
        xor     a
        ld      -22(ix), a
        ld      -21(ix), a
        ld      -20(ix), a
        ld      -19(ix), a
        ld      -18(ix), a
        ld      -17(ix), a
        ld      -16(ix), a
        ld      -15(ix), a
        ld      -14(ix), a
        ld      -13(ix), a
        ld      -12(ix), a
        ld      -11(ix), a
        ld      -10(ix), a
        ld      -9(ix), a

        ld      b, #53
.mul_loop:
        bit     0, -43(ix)      ; mplier bit0
        jp      z, .no_add
        ; acc += mcand  (14 bytes)
        or      a
        ld      a, -22(ix)
        adc     a, -36(ix)
        ld      -22(ix), a
        ld      a, -21(ix)
        adc     a, -35(ix)
        ld      -21(ix), a
        ld      a, -20(ix)
        adc     a, -34(ix)
        ld      -20(ix), a
        ld      a, -19(ix)
        adc     a, -33(ix)
        ld      -19(ix), a
        ld      a, -18(ix)
        adc     a, -32(ix)
        ld      -18(ix), a
        ld      a, -17(ix)
        adc     a, -31(ix)
        ld      -17(ix), a
        ld      a, -16(ix)
        adc     a, -30(ix)
        ld      -16(ix), a
        ld      a, -15(ix)
        adc     a, -29(ix)
        ld      -15(ix), a
        ld      a, -14(ix)
        adc     a, -28(ix)
        ld      -14(ix), a
        ld      a, -13(ix)
        adc     a, -27(ix)
        ld      -13(ix), a
        ld      a, -12(ix)
        adc     a, -26(ix)
        ld      -12(ix), a
        ld      a, -11(ix)
        adc     a, -25(ix)
        ld      -11(ix), a
        ld      a, -10(ix)
        adc     a, -24(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        adc     a, -23(ix)
        ld      -9(ix), a
.no_add:
        ; mcand <<= 1 (14 bytes)
        sla     -36(ix)
        rl      -35(ix)
        rl      -34(ix)
        rl      -33(ix)
        rl      -32(ix)
        rl      -31(ix)
        rl      -30(ix)
        rl      -29(ix)
        rl      -28(ix)
        rl      -27(ix)
        rl      -26(ix)
        rl      -25(ix)
        rl      -24(ix)
        rl      -23(ix)
        ; mplier >>= 1 (7 bytes)
        srl     -37(ix)
        rr      -38(ix)
        rr      -39(ix)
        rr      -40(ix)
        rr      -41(ix)
        rr      -42(ix)
        rr      -43(ix)
        dec     b
        jp      nz, .mul_loop

        ; Product in acc (14 bytes). Leading 1 at bit104 or bit105.
        ; V = acc bytes 6..13 = -16..-9 (V byte0 = acc[6] = -16).
        ; bit105 = acc bit105 = acc[13] bit1 = -9(ix) bit1.
        bit     1, -9(ix)
        jp      z, .lead104
        ; leading at 105: shift V right 5, exp++
        ld      b, #5
.sr105:
        srl     -9(ix)
        rr      -10(ix)
        rr      -11(ix)
        rr      -12(ix)
        rr      -13(ix)
        rr      -14(ix)
        rr      -15(ix)
        rr      -16(ix)
        djnz    .sr105
        ld      a, -45(ix)
        add     a, #1
        ld      -45(ix), a
        ld      a, -44(ix)
        adc     a, #0
        ld      -44(ix), a
        jp      .extract
.lead104:
        ; leading at 104: shift V right 4
        ld      b, #4
.sr104:
        srl     -9(ix)
        rr      -10(ix)
        rr      -11(ix)
        rr      -12(ix)
        rr      -13(ix)
        rr      -14(ix)
        rr      -15(ix)
        rr      -16(ix)
        djnz    .sr104
.extract:
        ; significand low 7 bytes = V[0..6] = -16..-10. bit52 = -10 bit4.
        ; pack result
        ld      e, -16(ix)      ; byte0
        ld      d, -15(ix)      ; byte1
        ld      l, -14(ix)      ; byte2
        ld      h, -13(ix)      ; byte3
        exx
        ld      e, -12(ix)      ; byte4
        ld      d, -11(ix)      ; byte5
        exx
        ; byte6 = (exp[3:0]<<4) | (V[6] & 0x0F)
        ld      a, -45(ix)
        and     #0x0F
        rlca
        rlca
        rlca
        rlca
        ld      b, a
        ld      a, -10(ix)
        and     #0x0F
        or      b
        exx
        ld      l, a            ; byte6
        exx
        ; byte7 = sign | exp[10:4]
        ld      a, -45(ix)
        srl     a
        srl     a
        srl     a
        srl     a
        ld      b, a
        ld      a, -44(ix)
        add     a, a
        add     a, a
        add     a, a
        add     a, a
        or      b
        and     #0x7F
        ld      b, a
        ld      a, -46(ix)
        or      b
        exx
        ld      h, a            ; byte7
        exx
        ld      sp, ix
        pop     ix
        ret

.ret_zero:
        ld      sp, ix
        pop     ix
        jp      __db_zero
