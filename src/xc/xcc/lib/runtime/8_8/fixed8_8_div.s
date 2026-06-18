        ; fixed8_8_div.s
        ;
        ; Signed 8.8 fixed-point divide:
        ;   result = (a << 8) / b
        ;
        ; This first Z80 implementation uses a compact restoring 24/16 divider.
        ; Division by zero returns zero; callers that need traps can wrap it.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_div
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_div

        .area   _CODE

        ; _fixed8_8_div
        ; inputs:  HL = a, DE = b
        ; outputs: DE = signed ((a << 8) / b), truncated toward zero
        ; clobbers: AF, BC, HL, IX
_fixed8_8_div::
        ld      a,d
        or      e
        jr      nz,.nonzero
        ld      de,#0
        ret

.nonzero:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      bc,#0                   ; C bit 0 tracks quotient sign

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
        ; Frame:
        ;   -7     = sign flag
        ;   -6..-3 dividend/quotient q = abs(a) << 8, little endian
        ;   -2..-1 remainder r, little endian
        ld      a,c
        ld      c,l
        ld      b,h
        ld      hl,#-7
        add     hl,sp
        ld      sp,hl

        ld      -7(ix),a
        ld      -6(ix),#0
        ld      -5(ix),c
        ld      -4(ix),b
        ld      -3(ix),#0
        ld      -2(ix),#0
        ld      -1(ix),#0

        ld      b,#24
.div_loop:
        sla     -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)

        rl      -2(ix)
        rl      -1(ix)

        or      a
        ld      a,-2(ix)
        sbc     a,e
        ld      -2(ix),a
        ld      a,-1(ix)
        sbc     a,d
        ld      -1(ix),a
        jr      nc,.keep_sub

        or      a
        ld      a,-2(ix)
        adc     a,e
        ld      -2(ix),a
        ld      a,-1(ix)
        adc     a,d
        ld      -1(ix),a
        jr      .next_bit
.keep_sub:
        set     0,-6(ix)
.next_bit:
        djnz    .div_loop

        ; Low 16 bits of q are the fixed-point result.
        ld      e,-6(ix)
        ld      d,-5(ix)

        ld      a,-7(ix)
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
