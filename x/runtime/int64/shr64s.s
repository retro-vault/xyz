        ; 64-bit arithmetic right shift
        ; a in DE:HL:DE':HL', count at ix+4
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module shr64s
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __shr64s

__shr64s:
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      b, h
        ld      c, l

        ld      hl, #-9
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

        ld      a, -1(ix)
        and     #0x80
        jr      z, .fill_zero
        ld      a, #0xFF
        jr      .fill_set
.fill_zero:
        xor     a
.fill_set:
        ld      -9(ix), a

        ld      a, 4(ix)
        or      a
        jp      z, .done
        cp      #64
        jp      c, .word_shifts
        ld      a, -9(ix)
        ld      -8(ix), a
        ld      -7(ix), a
        ld      -6(ix), a
        ld      -5(ix), a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a
        jp      .done

.word_shifts:
        cp      #16
        jp      c, .bit_shifts
        sub     #16
        push    af

        ld      b, -6(ix)
        ld      c, -5(ix)
        ld      -8(ix), b
        ld      -7(ix), c

        ld      b, -4(ix)
        ld      c, -3(ix)
        ld      -6(ix), b
        ld      -5(ix), c

        ld      b, -2(ix)
        ld      c, -1(ix)
        ld      -4(ix), b
        ld      -3(ix), c

        ld      a, -9(ix)
        ld      -2(ix), a
        ld      -1(ix), a
        pop     af
        jp      .word_shifts

.bit_shifts:
        ld      b, a
        ld      a, b
        or      a
        jp      z, .done

.bit_loop:
        ld      a, -1(ix)
        sra     a
        ld      -1(ix), a
        ld      a, -2(ix)
        rra
        ld      -2(ix), a
        ld      a, -3(ix)
        rra
        ld      -3(ix), a
        ld      a, -4(ix)
        rra
        ld      -4(ix), a
        ld      a, -5(ix)
        rra
        ld      -5(ix), a
        ld      a, -6(ix)
        rra
        ld      -6(ix), a
        ld      a, -7(ix)
        rra
        ld      -7(ix), a
        ld      a, -8(ix)
        rra
        ld      -8(ix), a
        djnz    .bit_loop

.done:
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
