        ; fixed16_16_mul.s
        ;
        ; Signed 16.16 fixed-point multiply:
        ;   result = (a * b) >> 16
        ;
        ; Public ABI is int32 raw fixed (DE low16, HL high16).  The wider
        ; product is held only as local byte scratch, so libfixed does not
        ; depend on the generic wide-integer runtime helpers.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_mul
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_mul

        .area   _CODE

        ; locals:
        ;   -25        sign flag
        ;   -24..-17   multiplicand64, low..high
        ;   -16..-13   multiplier32, low..high
        ;   -12..-5    product64, low..high

        ; inputs:  DE:HL = a, 4(ix)..7(ix) = b
        ; outputs: DE:HL = signed ((a * b) >> 16)
_fixed16_16_mul::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      b,h
        ld      c,l
        ld      hl,#-25
        add     hl,sp
        ld      sp,hl
        ld      h,b
        ld      l,c

        xor     a
        ld      -25(ix),a

        bit     7,h
        jr      z,.a_abs_done
        ld      -25(ix),#1
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ld      a,#0
        sbc     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
.a_abs_done:

        ld      a,4(ix)
        ld      -16(ix),a
        ld      a,5(ix)
        ld      -15(ix),a
        ld      a,6(ix)
        ld      -14(ix),a
        ld      a,7(ix)
        ld      -13(ix),a

        bit     7,-13(ix)
        jr      z,.b_abs_done
        ld      a,-25(ix)
        xor     #1
        ld      -25(ix),a
        xor     a
        sub     a,-16(ix)
        ld      -16(ix),a
        ld      a,#0
        sbc     a,-15(ix)
        ld      -15(ix),a
        ld      a,#0
        sbc     a,-14(ix)
        ld      -14(ix),a
        ld      a,#0
        sbc     a,-13(ix)
        ld      -13(ix),a
.b_abs_done:

        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h
        xor     a
        ld      -20(ix),a
        ld      -19(ix),a
        ld      -18(ix),a
        ld      -17(ix),a

        ld      -12(ix),a
        ld      -11(ix),a
        ld      -10(ix),a
        ld      -9(ix),a
        ld      -8(ix),a
        ld      -7(ix),a
        ld      -6(ix),a
        ld      -5(ix),a

        ld      b,#32
.mul_loop:
        bit     0,-16(ix)
        jr      z,.skip_add

        ld      a,-12(ix)
        add     a,-24(ix)
        ld      -12(ix),a
        ld      a,-11(ix)
        adc     a,-23(ix)
        ld      -11(ix),a
        ld      a,-10(ix)
        adc     a,-22(ix)
        ld      -10(ix),a
        ld      a,-9(ix)
        adc     a,-21(ix)
        ld      -9(ix),a
        ld      a,-8(ix)
        adc     a,-20(ix)
        ld      -8(ix),a
        ld      a,-7(ix)
        adc     a,-19(ix)
        ld      -7(ix),a
        ld      a,-6(ix)
        adc     a,-18(ix)
        ld      -6(ix),a
        ld      a,-5(ix)
        adc     a,-17(ix)
        ld      -5(ix),a

.skip_add:
        sla     -24(ix)
        rl      -23(ix)
        rl      -22(ix)
        rl      -21(ix)
        rl      -20(ix)
        rl      -19(ix)
        rl      -18(ix)
        rl      -17(ix)

        srl     -13(ix)
        rr      -14(ix)
        rr      -15(ix)
        rr      -16(ix)

        djnz    .mul_loop

        ld      a,-25(ix)
        or      a
        jr      z,.shift_result
        xor     a
        sub     a,-12(ix)
        ld      -12(ix),a
        ld      a,#0
        sbc     a,-11(ix)
        ld      -11(ix),a
        ld      a,#0
        sbc     a,-10(ix)
        ld      -10(ix),a
        ld      a,#0
        sbc     a,-9(ix)
        ld      -9(ix),a
        ld      a,#0
        sbc     a,-8(ix)
        ld      -8(ix),a
        ld      a,#0
        sbc     a,-7(ix)
        ld      -7(ix),a
        ld      a,#0
        sbc     a,-6(ix)
        ld      -6(ix),a
        ld      a,#0
        sbc     a,-5(ix)
        ld      -5(ix),a

.shift_result:
        ld      b,#16
.shift_loop:
        ld      a,-5(ix)
        sra     a
        ld      -5(ix),a
        ld      a,-6(ix)
        rra
        ld      -6(ix),a
        ld      a,-7(ix)
        rra
        ld      -7(ix),a
        ld      a,-8(ix)
        rra
        ld      -8(ix),a
        ld      a,-9(ix)
        rra
        ld      -9(ix),a
        ld      a,-10(ix)
        rra
        ld      -10(ix),a
        ld      a,-11(ix)
        rra
        ld      -11(ix),a
        ld      a,-12(ix)
        rra
        ld      -12(ix),a
        djnz    .shift_loop

        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        ld      sp,ix
        pop     ix
        ret
