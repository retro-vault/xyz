        ; Shared 16-bit unsigned divide core.
        ; Adapted from retro-vault/libsdcc-z80 `src/int/divunsigned.s`.
        ;
        ; ABI:

        .module divunsigned
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __divuint

        ; __divuint
        ; inputs: hl = dividend, de = divisor
        ; outputs: de = quotient, hl = remainder
        ; clobbers: a, b, d, e, h, l, f

__divuint:
divu16_core:
        ld      a, e
        and     a, #0x80
        or      a, d
        jr      nz, .morethan7bits

.atmost7bits:
        ld      b, #16
        adc     hl, hl
.dvloop7:
        rla
        sub     a, e
        jr      nc, .nodrop7
        add     a, e
.nodrop7:
        ccf
        adc     hl, hl
        djnz    .dvloop7
        ld      e, a
        ex      de, hl
        ret

.morethan7bits:
        ld      b, #9
        ld      a, l
        ld      l, h
        ld      h, #0
        rr      l
.dvloop:
        adc     hl, hl
        sbc     hl, de
        jr      nc, .nodrop
        add     hl, de
.nodrop:
        ccf
        rla
        djnz    .dvloop
        rl      b
        ld      d, b
        ld      e, a
        ret
