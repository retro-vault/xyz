        ; signed 32-bit modulus (long), reentrant
        ;
        ; ABI (sdcccall(1), matches your build):
        ;   x (dividend) in regs:  DE = low16, HL = high16
        ;   y (divisor)  on stack: 4(ix)..7(ix) = y0..y3 (lsb..msb)
        ; returns:
        ;   remainder in regs:     DE = low16, HL = high16
        ;
        ; semantics:
        ;   r = x % y with sign(r) == sign(x)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module modslong
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  __smod32
        .globl  __modslong_rrx_s
        .globl  __modslong_rrf_s
        .globl  __modslong

        ; locals (relative to ix):
        ;   -1      : sign_x (0/1) = sign(x)
        ;   -6..-3  : abs(y) (low..high)
        ;   -10..-7 : remainder (low..high)
        ; __modslong
        ; inputs:  x in DE:HL (signed), y at 4(ix)..7(ix) (signed,
        ; lsb..msb)
        ; outputs: DE:HL = x % y, sign(remainder)=sign(x)
        ; clobbers: af, bc, de, hl, ix
__smod32:
__modslong_rrx_s::
__modslong_rrf_s::
__modslong:
        push    ix
        ld      ix, #0
        add     ix, sp

        ; save incoming x.high (HL) into BC; HL used for frame math
        ld      b, h
        ld      c, l

        ; reserve 10 bytes locals
        ld      hl, #-10
        add     hl, sp
        ld      sp, hl

        ; restore HL (x.high)
        ld      h, b
        ld      l, c

        xor     a
        ld      -1(ix), a

        ; normalize x into internal order DE:HL = high:low
        ; incoming: DE low, HL high -> internal: DE high, HL low
        ex      de, hl

        ; abs(x), save original sign
        bit     7, d
        jr      z, .x_abs_done
        ld      -1(ix), #1
        call    .neg_dehl
.x_abs_done:

        ; copy y from stack into abs(y) locals -6..-3 (low..high)
        ld      a, 4(ix)
        ld      -6(ix), a
        ld      a, 5(ix)
        ld      -5(ix), a
        ld      a, 6(ix)
        ld      -4(ix), a
        ld      a, 7(ix)
        ld      -3(ix), a

        ; abs(y) if negative
        bit     7, -3(ix)
        jr      z, .y_abs_done
        xor     a
        sub     a, -6(ix)
        ld      -6(ix), a
        ld      a, #0
        sbc     a, -5(ix)
        ld      -5(ix), a
        ld      a, #0
        sbc     a, -4(ix)
        ld      -4(ix), a
        ld      a, #0
        sbc     a, -3(ix)
        ld      -3(ix), a
.y_abs_done:

        ; remainder = 0 at -10..-7
        xor     a
        ld      -10(ix), a
        ld      -9(ix),  a
        ld      -8(ix),  a
        ld      -7(ix),  a

        ; unsigned restoring division (remainder only)
        ld      b, #32

.u32_mod_loop:
        ; shift dividend (abs x) left by 1
        add     hl, hl
        rl      e
        rl      d

        ; remainder <<= 1, bring in carry from x shift
        rl      -10(ix)
        rl      -9(ix)
        rl      -8(ix)
        rl      -7(ix)

        ; try: remainder -= abs(y)
        or      a
        ld      a, -10(ix)
        sbc     a, -6(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        sbc     a, -5(ix)
        ld      -9(ix), a
        ld      a, -8(ix)
        sbc     a, -4(ix)
        ld      -8(ix), a
        ld      a, -7(ix)
        sbc     a, -3(ix)
        ld      -7(ix), a
        jr      nc, .next_bit

        ; borrow -> restore remainder
        or      a
        ld      a, -10(ix)
        adc     a, -6(ix)
        ld      -10(ix), a
        ld      a, -9(ix)
        adc     a, -5(ix)
        ld      -9(ix), a
        ld      a, -8(ix)
        adc     a, -4(ix)
        ld      -8(ix), a
        ld      a, -7(ix)
        adc     a, -3(ix)
        ld      -7(ix), a

.next_bit:
        djnz    .u32_mod_loop

        ; restore remainder sign to sign(x)
        ld      a, -1(ix)
        or      a
        jr      z, .ret_rem

        xor     a
        sub     a, -10(ix)
        ld      -10(ix), a
        ld      a, #0
        sbc     a, -9(ix)
        ld      -9(ix), a
        ld      a, #0
        sbc     a, -8(ix)
        ld      -8(ix), a
        ld      a, #0
        sbc     a, -7(ix)
        ld      -7(ix), a

.ret_rem:
        ; return remainder in ABI order DE low, HL high
        ld      e, -10(ix)
        ld      d, -9(ix)
        ld      l, -8(ix)
        ld      h, -7(ix)

        ld      sp, ix
        pop     ix
        ret

.neg_dehl:
        xor     a
        sub     a, l
        ld      l, a
        ld      a, #0
        sbc     a, h
        ld      h, a
        ld      a, #0
        sbc     a, e
        ld      e, a
        ld      a, #0
        sbc     a, d
        ld      d, a
        ret
