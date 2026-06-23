        ; unsigned 64-bit divide
        ;
        ; Algorithm: restoring division, 64 iterations.
        ;
        ; Frame layout (24 bytes):
        ;   ix-8..ix-1:   quotient/dividend q (lsb..msb)
        ;   ix-16..ix-9:  divisor d (lsb..msb)
        ;   ix-24..ix-17: remainder r (lsb..msb)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module divull
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __divull
        .globl  __div64

__div64:
__divull:
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

        ; q = a
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

        ; d = b
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
        ld      -24(ix), a
        ld      -23(ix), a
        ld      -22(ix), a
        ld      -21(ix), a
        ld      -20(ix), a
        ld      -19(ix), a
        ld      -18(ix), a
        ld      -17(ix), a

        ld      b, #64
.divull_loop:
        ld      a, -8(ix)
        add     a, a
        ld      -8(ix), a
        ld      a, -7(ix)
        rla
        ld      -7(ix), a
        ld      a, -6(ix)
        rla
        ld      -6(ix), a
        ld      a, -5(ix)
        rla
        ld      -5(ix), a
        ld      a, -4(ix)
        rla
        ld      -4(ix), a
        ld      a, -3(ix)
        rla
        ld      -3(ix), a
        ld      a, -2(ix)
        rla
        ld      -2(ix), a
        ld      a, -1(ix)
        rla
        ld      -1(ix), a

        ld      a, -24(ix)
        rla
        ld      -24(ix), a
        ld      a, -23(ix)
        rla
        ld      -23(ix), a
        ld      a, -22(ix)
        rla
        ld      -22(ix), a
        ld      a, -21(ix)
        rla
        ld      -21(ix), a
        ld      a, -20(ix)
        rla
        ld      -20(ix), a
        ld      a, -19(ix)
        rla
        ld      -19(ix), a
        ld      a, -18(ix)
        rla
        ld      -18(ix), a
        ld      a, -17(ix)
        rla
        ld      -17(ix), a

        or      a
        ld      a, -24(ix)
        sbc     a, -16(ix)
        ld      -24(ix), a
        ld      a, -23(ix)
        sbc     a, -15(ix)
        ld      -23(ix), a
        ld      a, -22(ix)
        sbc     a, -14(ix)
        ld      -22(ix), a
        ld      a, -21(ix)
        sbc     a, -13(ix)
        ld      -21(ix), a
        ld      a, -20(ix)
        sbc     a, -12(ix)
        ld      -20(ix), a
        ld      a, -19(ix)
        sbc     a, -11(ix)
        ld      -19(ix), a
        ld      a, -18(ix)
        sbc     a, -10(ix)
        ld      -18(ix), a
        ld      a, -17(ix)
        sbc     a, -9(ix)
        ld      -17(ix), a
        jr      nc, .divull_keep

        or      a
        ld      a, -24(ix)
        adc     a, -16(ix)
        ld      -24(ix), a
        ld      a, -23(ix)
        adc     a, -15(ix)
        ld      -23(ix), a
        ld      a, -22(ix)
        adc     a, -14(ix)
        ld      -22(ix), a
        ld      a, -21(ix)
        adc     a, -13(ix)
        ld      -21(ix), a
        ld      a, -20(ix)
        adc     a, -12(ix)
        ld      -20(ix), a
        ld      a, -19(ix)
        adc     a, -11(ix)
        ld      -19(ix), a
        ld      a, -18(ix)
        adc     a, -10(ix)
        ld      -18(ix), a
        ld      a, -17(ix)
        adc     a, -9(ix)
        ld      -17(ix), a
        jr      .divull_next

.divull_keep:
        set     0, -8(ix)

.divull_next:
        dec     b
        jp      nz, .divull_loop

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
