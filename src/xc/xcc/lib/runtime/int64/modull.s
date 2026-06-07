        ; unsigned 64-bit modulo — reuses __divull core, returns remainder
        ;
        ; Frame layout same as divull (16 bytes):
        ;   ix-8..ix-1:  dividend/quotient (discarded)
        ;   ix-16..ix-9: remainder (returned)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module modull
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __modull
        .globl  __mod64

        ; __modull / __mod64
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ; outputs: DE:HL:DE':HL' = unsigned remainder
        ; clobbers: af, bc, de, hl, ix, de', hl'

__mod64:
__modull:
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
.modull_loop:
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
        jp      nc, .modull_keep

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
        jp      .modull_next

.modull_keep:
        set     0, -8(ix)

.modull_next:
        dec     b
        jp      nz, .modull_loop

        ; remainder is at ix-16..ix-9
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
