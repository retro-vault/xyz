        ; ieee-754 double add:  result = a + b
        ; a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ;
        ; denormals (exp==0) flushed to 0; no NaN/Inf; truncation.
        ;
        ; Frame (negative offsets from ix), 28 bytes:
        ;   -8 ..-1  : a bytes a0..a7
        ;   -16..-9  : mant_x (larger operand significand) m0..m7
        ;   -24..-17 : mant_y (smaller operand significand) m0..m7
        ;   -26..-25 : result exponent (lo at -26, hi at -25)
        ;   -27      : result sign
        ;   -28      : shift count
        ;
        ; significand layout: implicit 1 at bit52 = m6 bit4.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbadd
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __dbadd
        .globl  ___dbadd
        .globl  __db_zero

__dbadd:
___dbadd::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-28
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

        call    .exp_a_to_hl    ; HL = exp_a
        ld      d, h
        ld      e, l            ; DE = exp_a
        ld      a, d
        or      e
        jp      nz, .a_ok
        ; exp_a == 0 → return b
        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, 6(ix)
        ld      h, 7(ix)
        exx
        ld      e, 8(ix)
        ld      d, 9(ix)
        ld      l, 10(ix)
        ld      h, 11(ix)
        exx
        jp      .done
.a_ok:
        call    .exp_b_to_hl    ; HL = exp_b
        ld      a, h
        or      l
        jp      nz, .b_ok
        ; exp_b == 0 → return a
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
        jp      .done
.b_ok:
        ; DE = exp_a, HL = exp_b. Decide larger.
        ld      a, d
        cp      h
        jp      c, .b_larger
        jp      nz, .a_larger
        ld      a, e
        cp      l
        jp      c, .b_larger
        jp      nz, .a_larger
        ; equal exponents: compare mantissa bytes (a6&0xF..a0) vs (b6&0xF..b0)
        ld      a, -2(ix)
        and     #0x0F
        ld      b, a
        ld      a, 10(ix)
        and     #0x0F
        cp      b
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 9(ix)
        cp      -3(ix)
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 8(ix)
        cp      -4(ix)
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 7(ix)
        cp      -5(ix)
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 6(ix)
        cp      -6(ix)
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 5(ix)
        cp      -7(ix)
        jp      c, .a_larger
        jp      nz, .b_larger
        ld      a, 4(ix)
        cp      -8(ix)
        jp      c, .a_larger
        ; equal or a>=b → a larger
.a_larger:
        ld      -25(ix), d
        ld      -26(ix), e
        ld      a, -1(ix)
        and     #0x80
        ld      -27(ix), a      ; sign_x = sign_a
        ld      a, e
        sub     l
        ld      -28(ix), a      ; shift = exp_a - exp_b
        call    .build_x_from_a
        call    .build_y_from_b
        jp      .have_xy
.b_larger:
        ld      -25(ix), h
        ld      -26(ix), l
        ld      a, 11(ix)
        and     #0x80
        ld      -27(ix), a      ; sign_x = sign_b
        ld      a, l
        sub     e
        ld      -28(ix), a
        call    .build_x_from_b
        call    .build_y_from_a
.have_xy:
        ; if shift >= 53, y negligible → result = x
        ld      a, -28(ix)
        cp      #53
        jp      nc, .pack
        ; shift mant_y right by shift
        or      a
        jp      z, .aligned
        ld      b, a
.shy:
        srl     -17(ix)
        rr      -18(ix)
        rr      -19(ix)
        rr      -20(ix)
        rr      -21(ix)
        rr      -22(ix)
        rr      -23(ix)
        rr      -24(ix)
        djnz    .shy
.aligned:
        ; add iff sign_a == sign_b
        ld      a, -1(ix)
        and     #0x80
        ld      c, a            ; sign_a
        ld      a, 11(ix)
        and     #0x80
        xor     c
        and     #0x80
        jp      nz, .do_sub

        ; ADD mant_x += mant_y
        or      a
        ld      a, -16(ix)
        add     a, -24(ix)
        ld      -16(ix), a
        ld      a, -15(ix)
        adc     a, -23(ix)
        ld      -15(ix), a
        ld      a, -14(ix)
        adc     a, -22(ix)
        ld      -14(ix), a
        ld      a, -13(ix)
        adc     a, -21(ix)
        ld      -13(ix), a
        ld      a, -12(ix)
        adc     a, -20(ix)
        ld      -12(ix), a
        ld      a, -11(ix)
        adc     a, -19(ix)
        ld      -11(ix), a
        ld      a, -10(ix)
        adc     a, -18(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        adc     a, -17(ix)
        ld      -9(ix), a
        ; if bit53 (m6 bit5) set → shift right 1, exp++
        bit     5, -10(ix)
        jp      z, .pack
        srl     -9(ix)
        rr      -10(ix)
        rr      -11(ix)
        rr      -12(ix)
        rr      -13(ix)
        rr      -14(ix)
        rr      -15(ix)
        rr      -16(ix)
        ld      a, -26(ix)
        add     a, #1
        ld      -26(ix), a
        ld      a, -25(ix)
        adc     a, #0
        ld      -25(ix), a
        jp      .pack

.do_sub:
        ; SUBTRACT mant_x -= mant_y
        or      a
        ld      a, -16(ix)
        sbc     a, -24(ix)
        ld      -16(ix), a
        ld      a, -15(ix)
        sbc     a, -23(ix)
        ld      -15(ix), a
        ld      a, -14(ix)
        sbc     a, -22(ix)
        ld      -14(ix), a
        ld      a, -13(ix)
        sbc     a, -21(ix)
        ld      -13(ix), a
        ld      a, -12(ix)
        sbc     a, -20(ix)
        ld      -12(ix), a
        ld      a, -11(ix)
        sbc     a, -19(ix)
        ld      -11(ix), a
        ld      a, -10(ix)
        sbc     a, -18(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        sbc     a, -17(ix)
        ld      -9(ix), a
        ; zero?
        ld      a, -16(ix)
        or      -15(ix)
        or      -14(ix)
        or      -13(ix)
        or      -12(ix)
        or      -11(ix)
        or      -10(ix)
        or      -9(ix)
        jp      nz, .sub_norm
        ld      sp, ix
        pop     ix
        jp      __db_zero
.sub_norm:
        bit     4, -10(ix)
        jp      nz, .pack
        ; also guard: if anything above bit52 set (shouldn't happen on sub) stop
        sla     -16(ix)
        rl      -15(ix)
        rl      -14(ix)
        rl      -13(ix)
        rl      -12(ix)
        rl      -11(ix)
        rl      -10(ix)
        rl      -9(ix)
        ld      a, -26(ix)
        sub     #1
        ld      -26(ix), a
        ld      a, -25(ix)
        sbc     a, #0
        ld      -25(ix), a
        jp      .sub_norm

.pack:
        ld      e, -16(ix)
        ld      d, -15(ix)
        ld      l, -14(ix)
        ld      h, -13(ix)
        exx
        ld      e, -12(ix)
        ld      d, -11(ix)
        exx
        ld      a, -26(ix)
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
        ld      a, -26(ix)
        srl     a
        srl     a
        srl     a
        srl     a
        ld      b, a
        ld      a, -25(ix)
        add     a, a
        add     a, a
        add     a, a
        add     a, a
        or      b
        and     #0x7F
        ld      b, a
        ld      a, -27(ix)
        or      b
        exx
        ld      h, a            ; byte7
        exx
.done:
        ld      sp, ix
        pop     ix
        ret

        ; --- helpers ---
.exp_a_to_hl:
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
        ld      l, a
        ret
.exp_b_to_hl:
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
        ld      l, a
        ret
.build_x_from_a:
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
        ld      -10(ix), a
        xor     a
        ld      -9(ix), a
        ret
.build_x_from_b:
        ld      a, 4(ix)
        ld      -16(ix), a
        ld      a, 5(ix)
        ld      -15(ix), a
        ld      a, 6(ix)
        ld      -14(ix), a
        ld      a, 7(ix)
        ld      -13(ix), a
        ld      a, 8(ix)
        ld      -12(ix), a
        ld      a, 9(ix)
        ld      -11(ix), a
        ld      a, 10(ix)
        and     #0x0F
        or      #0x10
        ld      -10(ix), a
        xor     a
        ld      -9(ix), a
        ret
.build_y_from_b:
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
        ret
.build_y_from_a:
        ld      a, -8(ix)
        ld      -24(ix), a
        ld      a, -7(ix)
        ld      -23(ix), a
        ld      a, -6(ix)
        ld      -22(ix), a
        ld      a, -5(ix)
        ld      -21(ix), a
        ld      a, -4(ix)
        ld      -20(ix), a
        ld      a, -3(ix)
        ld      -19(ix), a
        ld      a, -2(ix)
        and     #0x0F
        or      #0x10
        ld      -18(ix), a
        xor     a
        ld      -17(ix), a
        ret
