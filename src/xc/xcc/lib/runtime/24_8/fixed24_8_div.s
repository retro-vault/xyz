        ; fixed24_8_div.s
        ;
        ; Signed 24.8 fixed-point divide:
        ;   result = (a << 8) / b
        ;
        ; Public ABI is int32 raw fixed (DE low16, HL high16).  The widened
        ; numerator and restoring divider state are local byte scratch only.
        ;
        ; Division by zero returns zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_div
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_div

        .area   _CODE

        ; locals:
        ;   -1         sign flag
        ;   -9..-2     quotient/dividend64, low..high
        ;   -17..-10   divisor64, low..high
        ;   -25..-18   remainder64, low..high

        ; inputs:  DE:HL = a, 4(ix)..7(ix) = b
        ; outputs: DE:HL = signed ((a << 8) / b)
_fixed24_8_div::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        jr      nz,.nonzero
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret

.nonzero:
        ld      b,h
        ld      c,l
        ld      hl,#-25
        add     hl,sp
        ld      sp,hl
        ld      h,b
        ld      l,c

        xor     a
        ld      -1(ix),a

        bit     7,h
        jr      z,.a_abs_done
        ld      -1(ix),#1
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

        ; q = abs(a) << 8
        xor     a
        ld      -9(ix),a
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        ld      -4(ix),a
        ld      -3(ix),a
        ld      -2(ix),a

        ld      a,4(ix)
        ld      -17(ix),a
        ld      a,5(ix)
        ld      -16(ix),a
        ld      a,6(ix)
        ld      -15(ix),a
        ld      a,7(ix)
        ld      -14(ix),a
        xor     a
        ld      -13(ix),a
        ld      -12(ix),a
        ld      -11(ix),a
        ld      -10(ix),a

        bit     7,-14(ix)
        jr      z,.b_abs_done
        ld      a,-1(ix)
        xor     #1
        ld      -1(ix),a
        xor     a
        sub     a,-17(ix)
        ld      -17(ix),a
        ld      a,#0
        sbc     a,-16(ix)
        ld      -16(ix),a
        ld      a,#0
        sbc     a,-15(ix)
        ld      -15(ix),a
        ld      a,#0
        sbc     a,-14(ix)
        ld      -14(ix),a
.b_abs_done:

        xor     a
        ld      -25(ix),a
        ld      -24(ix),a
        ld      -23(ix),a
        ld      -22(ix),a
        ld      -21(ix),a
        ld      -20(ix),a
        ld      -19(ix),a
        ld      -18(ix),a

        ld      b,#64
.div_loop:
        sla     -9(ix)
        rl      -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        rl      -2(ix)

        rl      -25(ix)
        rl      -24(ix)
        rl      -23(ix)
        rl      -22(ix)
        rl      -21(ix)
        rl      -20(ix)
        rl      -19(ix)
        rl      -18(ix)

        or      a
        ld      a,-25(ix)
        sbc     a,-17(ix)
        ld      -25(ix),a
        ld      a,-24(ix)
        sbc     a,-16(ix)
        ld      -24(ix),a
        ld      a,-23(ix)
        sbc     a,-15(ix)
        ld      -23(ix),a
        ld      a,-22(ix)
        sbc     a,-14(ix)
        ld      -22(ix),a
        ld      a,-21(ix)
        sbc     a,-13(ix)
        ld      -21(ix),a
        ld      a,-20(ix)
        sbc     a,-12(ix)
        ld      -20(ix),a
        ld      a,-19(ix)
        sbc     a,-11(ix)
        ld      -19(ix),a
        ld      a,-18(ix)
        sbc     a,-10(ix)
        ld      -18(ix),a
        jr      nc,.keep_sub

        or      a
        ld      a,-25(ix)
        adc     a,-17(ix)
        ld      -25(ix),a
        ld      a,-24(ix)
        adc     a,-16(ix)
        ld      -24(ix),a
        ld      a,-23(ix)
        adc     a,-15(ix)
        ld      -23(ix),a
        ld      a,-22(ix)
        adc     a,-14(ix)
        ld      -22(ix),a
        ld      a,-21(ix)
        adc     a,-13(ix)
        ld      -21(ix),a
        ld      a,-20(ix)
        adc     a,-12(ix)
        ld      -20(ix),a
        ld      a,-19(ix)
        adc     a,-11(ix)
        ld      -19(ix),a
        ld      a,-18(ix)
        adc     a,-10(ix)
        ld      -18(ix),a
        jr      .next_bit

.keep_sub:
        set     0,-9(ix)

.next_bit:
        dec     b
        jp      nz,.div_loop

        ld      a,-1(ix)
        or      a
        jr      nz,.ret_neg

        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        ld      sp,ix
        pop     ix
        ret

.ret_neg:
        xor     a
        sub     a,-9(ix)
        ld      e,a
        ld      a,#0
        sbc     a,-8(ix)
        ld      d,a
        ld      a,#0
        sbc     a,-7(ix)
        ld      l,a
        ld      a,#0
        sbc     a,-6(ix)
        ld      h,a
        ld      sp,ix
        pop     ix
        ret
