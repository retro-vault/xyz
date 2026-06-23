        ; ieee-754 double divide:  result = a / b
        ; a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; result in DE:HL:DE':HL'
        ;
        ; denormals treated as 0; divide-by-zero returns 0 (no inf).
        ;
        ; Frame (negative offsets from ix):
        ;   -8..-1  : a bytes a0..a7
        ;   -16..-9 : mant_a (dividend) 8 bytes, lsb..msb (53-bit significand)
        ;   -24..-17: mant_b (divisor)  8 bytes
        ;   -32..-25: remainder         8 bytes
        ;   -40..-33: quotient          8 bytes
        ;   -42..-41: result exp (16-bit)
        ;   -43     : result sign
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbdiv
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __dbdiv
        .globl  ___dbdiv
        .globl  __db_zero

        ; __dbdiv / ___dbdiv
__dbdiv:
___dbdiv::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-44
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
        ; save a bytes
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

        ; exponents
        ; exp_a = ((a7&0x7F)<<4)|(a6>>4)
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
        ; if exp_a == 0 → a is zero → return 0
        ld      a, h
        or      l
        jp      nz, .a_nz
        jp      .ret_zero
.a_nz:
        push    hl              ; save exp_a

        ; exp_b = ((b7&0x7F)<<4)|(b6>>4)   b7=11(ix), b6=10(ix)
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
        ; divide by zero → return 0 (no inf support)
        pop     hl
        jp      .ret_zero
.b_nz:
        ; result exp = exp_a - exp_b + 1023
        pop     de              ; DE = exp_a
        ex      de, hl          ; HL = exp_a, DE = exp_b
        or      a
        sbc     hl, de          ; HL = exp_a - exp_b
        ld      de, #1023
        add     hl, de          ; HL = result exp (biased, before normalize)
        ld      -42(ix), h
        ld      -41(ix), l

        ; result sign = sign_a XOR sign_b
        ld      a, -1(ix)
        xor     11(ix)
        and     #0x80
        ld      -43(ix), a

        ; mant_a (dividend): bytes m0..m6 from a0..a5 and (a6&0x0F)|0x10; m7=0
        ld      a, -8(ix)
        ld      -16(ix), a
        ld      a, -7(ix)
        ld      -15(ix), a
        ld      a, -6(ix)
        ld      -14(ix), a
        ld      a, -5(ix)
        ld      -13(ix), a
        ld      a, -4(ix)
        ld      -12(ix), a
        ld      a, -3(ix)
        ld      -11(ix), a
        ld      a, -2(ix)
        and     #0x0F
        or      #0x10
        ld      -10(ix), a      ; mant_a byte6 (bit4 = implicit 1)
        xor     a
        ld      -9(ix), a       ; mant_a byte7 = 0

        ; mant_b (divisor): from b0..b5 and (b6&0x0F)|0x10
        ld      a, 4(ix)
        ld      -24(ix), a
        ld      a, 5(ix)
        ld      -23(ix), a
        ld      a, 6(ix)
        ld      -22(ix), a
        ld      a, 7(ix)
        ld      -21(ix), a
        ld      a, 8(ix)
        ld      -20(ix), a
        ld      a, 9(ix)
        ld      -19(ix), a
        ld      a, 10(ix)
        and     #0x0F
        or      #0x10
        ld      -18(ix), a
        xor     a
        ld      -17(ix), a

        ; remainder = mant_a (dividend), quotient = 0
        ld      a, -16(ix)
        ld      -32(ix), a
        ld      a, -15(ix)
        ld      -31(ix), a
        ld      a, -14(ix)
        ld      -30(ix), a
        ld      a, -13(ix)
        ld      -29(ix), a
        ld      a, -12(ix)
        ld      -28(ix), a
        ld      a, -11(ix)
        ld      -27(ix), a
        ld      a, -10(ix)
        ld      -26(ix), a
        ld      a, -9(ix)
        ld      -25(ix), a
        xor     a
        ld      -40(ix), a
        ld      -39(ix), a
        ld      -38(ix), a
        ld      -37(ix), a
        ld      -36(ix), a
        ld      -35(ix), a
        ld      -34(ix), a
        ld      -33(ix), a

        ; Pre-reduce so rem < divisor (required by single-subtract loop).
        ; If mant_a >= mant_b (ratio >= 1): rem -= mant_b, set ge_flag.
        ; The integer "1" is re-injected at bit55 after the loop.
        ld      -44(ix), a      ; ge_flag = 0
        ; tentative subtract rem - divisor into a scratch (test only)
        or      a
        ld      a, -32(ix)
        sbc     a, -24(ix)
        ld      a, -31(ix)
        sbc     a, -23(ix)
        ld      a, -30(ix)
        sbc     a, -22(ix)
        ld      a, -29(ix)
        sbc     a, -21(ix)
        ld      a, -28(ix)
        sbc     a, -20(ix)
        ld      a, -27(ix)
        sbc     a, -19(ix)
        ld      a, -26(ix)
        sbc     a, -18(ix)
        ld      a, -25(ix)
        sbc     a, -17(ix)
        jp      c, .no_prereduce        ; rem < divisor → ratio < 1
        ; ratio >= 1: commit rem -= divisor, set flag
        ld      a, #1
        ld      -44(ix), a
        or      a
        ld      a, -32(ix)
        sbc     a, -24(ix)
        ld      -32(ix), a
        ld      a, -31(ix)
        sbc     a, -23(ix)
        ld      -31(ix), a
        ld      a, -30(ix)
        sbc     a, -22(ix)
        ld      -30(ix), a
        ld      a, -29(ix)
        sbc     a, -21(ix)
        ld      -29(ix), a
        ld      a, -28(ix)
        sbc     a, -20(ix)
        ld      -28(ix), a
        ld      a, -27(ix)
        sbc     a, -19(ix)
        ld      -27(ix), a
        ld      a, -26(ix)
        sbc     a, -18(ix)
        ld      -26(ix), a
        ld      a, -25(ix)
        sbc     a, -17(ix)
        ld      -25(ix), a
.no_prereduce:

        ; Long division: 55 iterations producing the 55 fraction bits.
        ; Each: quotient <<= 1; rem <<= 1;
        ;       if rem >= divisor: rem -= divisor, quotient |= 1.
        ld      b, #55
.div_loop:
        ; quotient <<= 1
        sla     -40(ix)
        rl      -39(ix)
        rl      -38(ix)
        rl      -37(ix)
        rl      -36(ix)
        rl      -35(ix)
        rl      -34(ix)
        rl      -33(ix)

        ; rem <<= 1  (rem initialised to mant_a; standard long division)
        sla     -32(ix)
        rl      -31(ix)
        rl      -30(ix)
        rl      -29(ix)
        rl      -28(ix)
        rl      -27(ix)
        rl      -26(ix)
        rl      -25(ix)

        ; compare rem >= divisor: subtract, restore if borrow
        or      a
        ld      a, -32(ix)
        sbc     a, -24(ix)
        ld      -32(ix), a
        ld      a, -31(ix)
        sbc     a, -23(ix)
        ld      -31(ix), a
        ld      a, -30(ix)
        sbc     a, -22(ix)
        ld      -30(ix), a
        ld      a, -29(ix)
        sbc     a, -21(ix)
        ld      -29(ix), a
        ld      a, -28(ix)
        sbc     a, -20(ix)
        ld      -28(ix), a
        ld      a, -27(ix)
        sbc     a, -19(ix)
        ld      -27(ix), a
        ld      a, -26(ix)
        sbc     a, -18(ix)
        ld      -26(ix), a
        ld      a, -25(ix)
        sbc     a, -17(ix)
        ld      -25(ix), a
        jp      nc, .div_keep

        ; restore: rem += divisor
        or      a
        ld      a, -32(ix)
        adc     a, -24(ix)
        ld      -32(ix), a
        ld      a, -31(ix)
        adc     a, -23(ix)
        ld      -31(ix), a
        ld      a, -30(ix)
        adc     a, -22(ix)
        ld      -30(ix), a
        ld      a, -29(ix)
        adc     a, -21(ix)
        ld      -29(ix), a
        ld      a, -28(ix)
        adc     a, -20(ix)
        ld      -28(ix), a
        ld      a, -27(ix)
        adc     a, -19(ix)
        ld      -27(ix), a
        ld      a, -26(ix)
        adc     a, -18(ix)
        ld      -26(ix), a
        ld      a, -25(ix)
        adc     a, -17(ix)
        ld      -25(ix), a
        jp      .div_next
.div_keep:
        set     0, -40(ix)      ; quotient |= 1
.div_next:
        dec     b
        jp      nz, .div_loop

        ; Round to nearest: if (rem << 1) >= divisor, increment quotient.
        ; This carries an all-ones quotient (e.g. 0xBFFF..F) up to the
        ; exact value (0xC000..0), restoring the leading bit.
        sla     -32(ix)
        rl      -31(ix)
        rl      -30(ix)
        rl      -29(ix)
        rl      -28(ix)
        rl      -27(ix)
        rl      -26(ix)
        rl      -25(ix)
        or      a
        ld      a, -32(ix)
        sbc     a, -24(ix)
        ld      a, -31(ix)
        sbc     a, -23(ix)
        ld      a, -30(ix)
        sbc     a, -22(ix)
        ld      a, -29(ix)
        sbc     a, -21(ix)
        ld      a, -28(ix)
        sbc     a, -20(ix)
        ld      a, -27(ix)
        sbc     a, -19(ix)
        ld      a, -26(ix)
        sbc     a, -18(ix)
        ld      a, -25(ix)
        sbc     a, -17(ix)
        jp      c, .no_round    ; 2*rem < divisor → no round
        ; quotient += 1
        ld      a, -40(ix)
        add     a, #1
        ld      -40(ix), a
        ld      a, -39(ix)
        adc     a, #0
        ld      -39(ix), a
        ld      a, -38(ix)
        adc     a, #0
        ld      -38(ix), a
        ld      a, -37(ix)
        adc     a, #0
        ld      -37(ix), a
        ld      a, -36(ix)
        adc     a, #0
        ld      -36(ix), a
        ld      a, -35(ix)
        adc     a, #0
        ld      -35(ix), a
        ld      a, -34(ix)
        adc     a, #0
        ld      -34(ix), a
        ld      a, -33(ix)
        adc     a, #0
        ld      -33(ix), a
.no_round:
        ; Re-inject the integer "1" at bit55 if ratio >= 1 (ge_flag set).
        ld      a, -44(ix)
        or      a
        jp      z, .norm
        set     7, -34(ix)      ; bit55 = q6 bit7

        ; Quotient now holds round(mant_a*2^55/mant_b).
        ; Q = mant_a*2^55/mant_b. Leading bit at 55 (ratio>=1) or 54 (ratio<1).
        ; The base exponent assumes the significand's leading 1 at bit52, so:
        ;   ratio >= 1 : shift right 3 (bit55 -> bit52), exp unchanged
        ;   ratio <  1 : shift right 2 (bit54 -> bit52), exp -= 1
        ; bit55 = q6 bit7 = -34(ix) bit7.
.norm:
        bit     7, -34(ix)
        jp      z, .ratio_lt1
        ld      b, #3
.nsr:
        srl     -33(ix)
        rr      -34(ix)
        rr      -35(ix)
        rr      -36(ix)
        rr      -37(ix)
        rr      -38(ix)
        rr      -39(ix)
        rr      -40(ix)
        djnz    .nsr
        jp      .packit
.ratio_lt1:
        ld      b, #2
.nsr2:
        srl     -33(ix)
        rr      -34(ix)
        rr      -35(ix)
        rr      -36(ix)
        rr      -37(ix)
        rr      -38(ix)
        rr      -39(ix)
        rr      -40(ix)
        djnz    .nsr2
        ld      a, -41(ix)
        sub     #1
        ld      -41(ix), a
        ld      a, -42(ix)
        sbc     a, #0
        ld      -42(ix), a

.packit:
        ; quotient bytes -40..-34 hold significand (bit52 = byte6 bit4 = 1).
        ; mant[51:48] = byte6 low nibble; clear bit4 (implicit 1).
        res     4, -34(ix)
        ; build result bytes into regs
        ; byte0 = -40, byte1 = -39, byte2 = -38, byte3 = -37
        ; byte4 = -36, byte5 = -35, byte6 = (exp[3:0]<<4)|mant[51:48], byte7 = sign|exp[10:4]
        ld      e, -40(ix)
        ld      d, -39(ix)
        ld      l, -38(ix)
        ld      h, -37(ix)
        exx
        ld      e, -36(ix)
        ld      d, -35(ix)
        exx

        ; byte6
        ld      a, -41(ix)      ; exp low byte
        and     #0x0F
        rlca
        rlca
        rlca
        rlca
        ld      b, a
        ld      a, -34(ix)
        and     #0x0F
        or      b
        exx
        ld      l, a            ; L' = byte6
        exx

        ; byte7 = sign | exp[10:4]
        ; exp in -42(hi),-41(lo)
        ld      a, -41(ix)
        srl     a
        srl     a
        srl     a
        srl     a
        ld      b, a            ; lo>>4
        ld      a, -42(ix)
        add     a, a
        add     a, a
        add     a, a
        add     a, a
        or      b
        and     #0x7F
        ld      b, a
        ld      a, -43(ix)      ; sign
        or      b
        exx
        ld      h, a            ; H' = byte7
        exx

        ld      sp, ix
        pop     ix
        ret

.ret_zero:
        ld      sp, ix
        pop     ix
        jp      __db_zero
