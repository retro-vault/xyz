        ; 64-bit multiply (low 64 bits only)
        ;
        ; Uses 16x16->32 partial products:
        ;   a = a0 + a1<<16 + a2<<32 + a3<<48
        ;   b = b0 + b1<<16 + b2<<32 + b3<<48
        ; low64(a*b) = sum(ai*bj << (16*(i+j))) for i+j < 4
        ;
        ; Since we keep only the low 64 bits, signed and unsigned two's-
        ; complement multiplication share the same bit-level result.
        ;
        ; Frame layout (24 bytes):
        ;   ix-24..ix-17 : a0..a3 (little-endian words, low to high)
        ;   ix-16..ix-9  : b0..b3 (little-endian words, low to high)
        ;   ix-8 ..ix-1  : result r0..r3 (little-endian words, low to high)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module mulll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mulll
        .globl  ___muluint2ulong

__mulll:
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      b, h
        ld      c, l

        ld      hl, #-24
        add     hl, sp
        ld      sp, hl

        ld      h, b
        ld      l, c

        ; save a words
        ld      -24(ix), e
        ld      -23(ix), d
        ld      -22(ix), l
        ld      -21(ix), h
        exx
        ld      -20(ix), e
        ld      -19(ix), d
        ld      -18(ix), l
        ld      -17(ix), h
        exx

        ; save b words
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
        ld      -10(ix), a
        ld      a, 11(ix)
        ld      -9(ix), a

        ; r = 0
        xor     a
        ld      -8(ix), a
        ld      -7(ix), a
        ld      -6(ix), a
        ld      -5(ix), a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a

        ; ------------------------------------------------------------
        ; k = 0: a0 * b0  -> add to r0..r3
        ; ------------------------------------------------------------
        ld      l, -24(ix)
        ld      h, -23(ix)
        ld      e, -16(ix)
        ld      d, -15(ix)
        call    ___muluint2ulong

        ld      a, -8(ix)
        add     a, e
        ld      -8(ix), a
        ld      a, -7(ix)
        adc     a, d
        ld      -7(ix), a
        ld      a, -6(ix)
        adc     a, l
        ld      -6(ix), a
        ld      a, -5(ix)
        adc     a, h
        ld      -5(ix), a
        ld      a, -4(ix)
        adc     a, #0
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, #0
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, #0
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, #0
        ld      -1(ix), a

        ; ------------------------------------------------------------
        ; k = 1: a0 * b1  -> add to r1..r3
        ; ------------------------------------------------------------
        ld      l, -24(ix)
        ld      h, -23(ix)
        ld      e, -14(ix)
        ld      d, -13(ix)
        call    ___muluint2ulong

        ld      a, -6(ix)
        add     a, e
        ld      -6(ix), a
        ld      a, -5(ix)
        adc     a, d
        ld      -5(ix), a
        ld      a, -4(ix)
        adc     a, l
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, h
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, #0
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, #0
        ld      -1(ix), a

        ; k = 1: a1 * b0
        ld      l, -22(ix)
        ld      h, -21(ix)
        ld      e, -16(ix)
        ld      d, -15(ix)
        call    ___muluint2ulong

        ld      a, -6(ix)
        add     a, e
        ld      -6(ix), a
        ld      a, -5(ix)
        adc     a, d
        ld      -5(ix), a
        ld      a, -4(ix)
        adc     a, l
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, h
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, #0
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, #0
        ld      -1(ix), a

        ; ------------------------------------------------------------
        ; k = 2: a0*b2 + a1*b1 + a2*b0  -> add to r2..r3
        ; ------------------------------------------------------------
        ld      l, -24(ix)
        ld      h, -23(ix)
        ld      e, -12(ix)
        ld      d, -11(ix)
        call    ___muluint2ulong

        ld      a, -4(ix)
        add     a, e
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, d
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, l
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, h
        ld      -1(ix), a

        ld      l, -22(ix)
        ld      h, -21(ix)
        ld      e, -14(ix)
        ld      d, -13(ix)
        call    ___muluint2ulong

        ld      a, -4(ix)
        add     a, e
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, d
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, l
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, h
        ld      -1(ix), a

        ld      l, -20(ix)
        ld      h, -19(ix)
        ld      e, -16(ix)
        ld      d, -15(ix)
        call    ___muluint2ulong

        ld      a, -4(ix)
        add     a, e
        ld      -4(ix), a
        ld      a, -3(ix)
        adc     a, d
        ld      -3(ix), a
        ld      a, -2(ix)
        adc     a, l
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, h
        ld      -1(ix), a

        ; ------------------------------------------------------------
        ; k = 3: a0*b3 + a1*b2 + a2*b1 + a3*b0 -> add low word to r3 only
        ; ------------------------------------------------------------
        ld      l, -24(ix)
        ld      h, -23(ix)
        ld      e, -10(ix)
        ld      d, -9(ix)
        call    ___muluint2ulong
        ld      a, -2(ix)
        add     a, e
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, d
        ld      -1(ix), a

        ld      l, -22(ix)
        ld      h, -21(ix)
        ld      e, -12(ix)
        ld      d, -11(ix)
        call    ___muluint2ulong
        ld      a, -2(ix)
        add     a, e
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, d
        ld      -1(ix), a

        ld      l, -20(ix)
        ld      h, -19(ix)
        ld      e, -14(ix)
        ld      d, -13(ix)
        call    ___muluint2ulong
        ld      a, -2(ix)
        add     a, e
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, d
        ld      -1(ix), a

        ld      l, -18(ix)
        ld      h, -17(ix)
        ld      e, -16(ix)
        ld      d, -15(ix)
        call    ___muluint2ulong
        ld      a, -2(ix)
        add     a, e
        ld      -2(ix), a
        ld      a, -1(ix)
        adc     a, d
        ld      -1(ix), a

        ; return r in DE:HL:DE':HL'
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

        ld      sp, ix
        pop     ix
        ret
