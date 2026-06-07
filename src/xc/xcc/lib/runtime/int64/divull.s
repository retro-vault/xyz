        ; unsigned 64-bit divide
        ;
        ; Algorithm: restoring division, 64 iterations.
        ; Dividend a occupies the same storage as the growing quotient:
        ;   bits are consumed from MSB and quotient bits fill from LSB.
        ;
        ; Frame layout (16 bytes):
        ;   ix-8..ix-1:  dividend/quotient (a0=lsb at ix-8, a7=msb at ix-1)
        ;   ix-16..ix-9: remainder (r0=lsb, r7=msb), init 0
        ; Divisor b at ix+4..ix+11 is read-only.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module divull
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __divull
        .globl  __div64

        ; __divull / __div64
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; outputs: DE:HL:DE':HL' = unsigned quotient
        ; clobbers: af, bc, de, hl, ix, de', hl'

__div64:
__divull:
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
.divull_loop:
        ; Shift dividend left: MSB goes to carry
        sla     -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        rl      -2(ix)
        rl      -1(ix)          ; carry = old bit63

        ; Shift remainder left with that carry into bit0
        rl      -16(ix)
        rl      -15(ix)
        rl      -14(ix)
        rl      -13(ix)
        rl      -12(ix)
        rl      -11(ix)
        rl      -10(ix)
        rl      -9(ix)

        ; Try: remainder -= divisor
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
        jp      nc, .divull_keep

        ; Borrow: restore remainder (add divisor back)
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
        jp      .divull_next

.divull_keep:
        set     0, -8(ix)       ; quotient bit = 1

.divull_next:
        dec     b
        jp      nz, .divull_loop

        ; quotient is at ix-8..ix-1
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
