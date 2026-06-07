        ; 64-bit multiply (unsigned/signed — same low-64-bit result)
        ;
        ; Algorithm: 64-iteration shift-add.
        ;   for each bit of multiplier a (shifted right): if set, acc += b (shifted left)
        ;
        ; Frame layout (16 bytes of locals):
        ;   ix-8..ix-1:  multiplier a0..a7 (lsb at ix-8, msb at ix-1)
        ;   ix-16..ix-9: accumulator (lsb at ix-16, msb at ix-9)
        ; Multiplicand b is shifted in-place on the stack (ix+4..ix+11).
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module mulll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mulll

        ; __mulll
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; outputs: DE:HL:DE':HL' = low 64 bits of a * b
        ; clobbers: af, bc, de, hl, ix, de', hl'

__mulll:
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      b, h
        ld      c, l

        ld      hl, #-16
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
.mulll_loop:
        bit     0, -8(ix)
        jp      z, .mulll_noadd

        or      a
        ld      a, -16(ix)
        add     a, 4(ix)
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

.mulll_noadd:
        sla     4(ix)
        rl      5(ix)
        rl      6(ix)
        rl      7(ix)
        rl      8(ix)
        rl      9(ix)
        rl      10(ix)
        rl      11(ix)

        srl     -1(ix)
        rr      -2(ix)
        rr      -3(ix)
        rr      -4(ix)
        rr      -5(ix)
        rr      -6(ix)
        rr      -7(ix)
        rr      -8(ix)

        dec     b
        jp      nz, .mulll_loop

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

        ld      sp, ix
        pop     ix
        ret
