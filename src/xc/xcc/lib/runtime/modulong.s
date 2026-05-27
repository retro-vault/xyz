        ; unsigned 32-bit modulus (long), reentrant
        ;
        ; ABI (sdcccall(1), matches your build):
        ; x (dividend) in regs:  DE = low16, HL = high16
        ; y (divisor)  on stack: 4(ix)..7(ix) = y0..y3 (lsb..msb)
        ; returns:
        ; remainder in regs:     DE = low16, HL = high16
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module modulong
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  __modulong_rrx_s
        .globl  __modulong_rrf_s
        .globl  __modulong

        ; __modulong
        ; inputs: x in DE:HL (DE=low16, HL=high16), y at 4(ix)..7(ix)
        ; (lsb..msb)
        ; outputs: DE:HL = x % y (DE=low16, HL=high16)
        ; clobbers: af, bc, de, hl, ix

__modulong_rrx_s::
__modulong_rrf_s::
__modulong:
        push    ix
        ld      ix, #0
        add     ix, sp

        ; save incoming x.high (HL) into BC; HL needed for frame math
        ld      b, h
        ld      c, l

        ; reserve 12 bytes locals
        ld      hl, #-12
        add     hl, sp
        ld      sp, hl

        ; restore HL (x.high)
        ld      h, b
        ld      l, c

        ; normalize dividend into internal order DE:HL = high:low
        ; incoming: DE low, HL high -> internal: DE high, HL low
        ex      de, hl

        ; remainder = 0
        xor     a
        ld      -8(ix), a
        ld      -7(ix), a
        ld      -6(ix), a
        ld      -5(ix), a

        ; copy divisor y into -12..-9 from 4..7(ix) (low..high)
        ld      a, 4(ix)
        ld      -12(ix), a
        ld      a, 5(ix)
        ld      -11(ix), a
        ld      a, 6(ix)
        ld      -10(ix), a
        ld      a, 7(ix)
        ld      -9(ix), a

        ; 32 iterations, restoring division (remainder only)
        ld      b, #32

.u32_mod_loop:
        ; shift dividend register left by 1 to feed next bit
        add     hl, hl
        rl      e
        rl      d

        ; remainder <<= 1, bring in carry from dividend shift
        rl      -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)

        ; try remainder -= divisor
        or      a
        ld      a, -8(ix)
        sbc     a, -12(ix)
        ld      -8(ix), a
        ld      a, -7(ix)
        sbc     a, -11(ix)
        ld      -7(ix), a
        ld      a, -6(ix)
        sbc     a, -10(ix)
        ld      -6(ix), a
        ld      a, -5(ix)
        sbc     a, -9(ix)
        ld      -5(ix), a
        jr      nc, .next_bit

        ; borrow -> restore remainder
        or      a
        ld      a, -8(ix)
        adc     a, -12(ix)
        ld      -8(ix), a
        ld      a, -7(ix)
        adc     a, -11(ix)
        ld      -7(ix), a
        ld      a, -6(ix)
        adc     a, -10(ix)
        ld      -6(ix), a
        ld      a, -5(ix)
        adc     a, -9(ix)
        ld      -5(ix), a

.next_bit:
        djnz    .u32_mod_loop

        ; return remainder in ABI order DE low, HL high
        ld      e, -8(ix)
        ld      d, -7(ix)
        ld      l, -6(ix)
        ld      h, -5(ix)

        ld      sp, ix
        pop     ix
        ret
