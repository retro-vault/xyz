        ; signed 32-bit multiply (low 32 bits)
        ;
        ; ABI (confirmed from caller):
        ; a in regs:  DE = low16, HL = high16
        ; b on stack: 4(ix)..7(ix) = b0..b3 (lsb..msb)
        ; returns:
        ; DE = low16, HL = high16
        ;
        ; implementation uses internal order DE:HL = high:low
        ; (bytes l,h,e,d are lsb..msb), so we swap at entry and swap
        ; back at return.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module mullong
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __mul32
        .globl  __mullong_rrx_s
        .globl  __mullong_rrf_s
        .globl  __mullong

        ; locals:
        ; -12..-9 : multiplier |b| (low..high)
        ; -8..-5  : accumulator (low 32)
        ; -13     : sign flag

        ; __mullong
        ; inputs: a in DE:HL, b at 4(ix)..7(ix) (lsb..msb)
        ; outputs: DE:HL = low 32 bits of signed product
        ; clobbers: af, bc, de, hl, ix

__mul32:
__mullong_rrx_s::
__mullong_rrf_s::
__mullong:
        push    ix
        ld      ix, #0
        add     ix, sp

        ; save incoming a.high (HL) into BC, because HL is needed for
        ; stack frame arithmetic below.
        ld      b, h
        ld      c, l

        ; reserve 13 bytes locals using HL as scratch
        ld      hl, #-13
        add     hl, sp
        ld      sp, hl

        ; restore incoming a.high back into HL
        ld      h, b
        ld      l, c

        ; now normalize a from ABI order (DE low, HL high)
        ; into internal order (DE high, HL low)
        ex      de, hl

        ; sign = sign(a) xor sign(b)
        xor     a
        ld      -13(ix), a
        ; sign(a) after swap
        bit     7, d
        jr      z, .a_nonneg
        ld      -13(ix), #1
.a_nonneg:
        ; b msb is at 7(ix)
        bit     7, 7(ix)
        jr      z, .sign_done
        ld      a, -13(ix)
        xor     #1
        ld      -13(ix), a
.sign_done:

        ; abs(a) if negative (a in de:hl = high:low)
        bit     7, d
        jr      z, .abs_a_done
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
.abs_a_done:

        ; copy b from stack 4..7 into locals -12..-9 (low..high)
        ld      a, 4(ix)
        ld      -12(ix), a
        ld      a, 5(ix)
        ld      -11(ix), a
        ld      a, 6(ix)
        ld      -10(ix), a
        ld      a, 7(ix)
        ld      -9(ix), a

        ; abs(b) if negative
        bit     7, -9(ix)
        jr      z, .abs_b_done
        xor     a
        sub     a, -12(ix)
        ld      -12(ix), a
        ld      a, #0
        sbc     a, -11(ix)
        ld      -11(ix), a
        ld      a, #0
        sbc     a, -10(ix)
        ld      -10(ix), a
        ld      a, #0
        sbc     a, -9(ix)
        ld      -9(ix), a
.abs_b_done:

        ; acc = 0
        xor     a
        ld      -8(ix), a
        ld      -7(ix), a
        ld      -6(ix), a
        ld      -5(ix), a

        ; shift-add 32 iterations
        ld      b, #32
.mul_loop:
        bit     0, -12(ix)
        jr      z, .no_add

        ; acc += multiplicand (bytes l,h,e,d)
        ld      a, -8(ix)
        add     a, l
        ld      -8(ix), a
        ld      a, -7(ix)
        adc     a, h
        ld      -7(ix), a
        ld      a, -6(ix)
        adc     a, e
        ld      -6(ix), a
        ld      a, -5(ix)
        adc     a, d
        ld      -5(ix), a

.no_add:
        ; multiplicand <<= 1
        add     hl, hl
        rl      e
        rl      d

        ; multiplier >>= 1 (msb -9 .. lsb -12)
        srl     -9(ix)
        rr      -10(ix)
        rr      -11(ix)
        rr      -12(ix)

        djnz    .mul_loop

        ; acc -> de:hl (internal order)
        ld      l, -8(ix)
        ld      h, -7(ix)
        ld      e, -6(ix)
        ld      d, -5(ix)

        ; apply sign if needed
        ld      a, -13(ix)
        or      a
        jr      z, .ret_order
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

.ret_order:
        ; tear down frame
        ld      sp, ix
        pop     ix

        ; convert internal (DE high, HL low) -> ABI return (DE low, HL
        ; high)
        ex      de, hl
        ret
