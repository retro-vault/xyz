        ; fixed8_8_mul.s
        ;
        ; Signed 8.8 fixed-point multiply:
        ;   result = (a * b) >> 8
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_mul
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_mul

        .area   _CODE

        ; _fixed8_8_mul
        ; inputs:  HL = a, DE = b
        ; outputs: DE = low 16 bits of signed ((a * b) >> 8)
        ; clobbers: AF, BC, HL, IX
_fixed8_8_mul::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      bc,#0                   ; C bit 0 tracks result sign

        bit     7,h
        jr      z,.a_abs_done
        xor     a
        sub     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
        inc     c
.a_abs_done:
        bit     7,d
        jr      z,.b_abs_done
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        inc     c
.b_abs_done:
        ld      a,c                     ; save sign before reusing BC
        ld      c,l
        ld      b,h

        ; Unsigned 16x16 -> 32-bit product in bytes:
        ;   -13    = sign flag
        ;   -12..-11 = multiplier, little endian
        ;   -10..-7  = product p0..p3, little endian
        ;   -6..-3   = multiplicand32, little endian
        ld      hl,#-13
        add     hl,sp
        ld      sp,hl

        ld      -13(ix),a
        ld      -12(ix),c
        ld      -11(ix),b

        xor     a
        ld      -10(ix),a
        ld      -9(ix),a
        ld      -8(ix),a
        ld      -7(ix),a

        ld      -6(ix),e
        ld      -5(ix),d
        ld      -4(ix),a
        ld      -3(ix),a

        ld      b,#16
.mul_loop:
        bit     0,-12(ix)
        jr      z,.skip_add
        ld      a,-10(ix)
        add     a,-6(ix)
        ld      -10(ix),a
        ld      a,-9(ix)
        adc     a,-5(ix)
        ld      -9(ix),a
        ld      a,-8(ix)
        adc     a,-4(ix)
        ld      -8(ix),a
        ld      a,-7(ix)
        adc     a,-3(ix)
        ld      -7(ix),a
.skip_add:
        sla     -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        srl     -11(ix)
        rr      -12(ix)
        djnz    .mul_loop

        ; Shift product right by 8: result = p1:p2 as little-endian DE.
        ld      e,-9(ix)
        ld      d,-8(ix)

        ld      a,-13(ix)
        ld      sp,ix
        bit     0,a
        jr      z,.done
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
.done:
        pop     ix
        ret
