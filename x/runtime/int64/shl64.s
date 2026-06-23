        ; 64-bit logical left shift
        ; a in DE:HL:DE':HL', count at ix+4 (low byte of stack arg)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module shl64
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __shl64

__shl64:
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      b, h
        ld      c, l

        ld      hl, #-8
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

        ld      a, 4(ix)
        or      a
        jp      z, .done
        cp      #64
        jp      c, .word_shifts
        xor     a
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

        ld      b, -4(ix)
        ld      c, -3(ix)
        ld      -2(ix), b
        ld      -1(ix), c

        ld      b, -6(ix)
        ld      c, -5(ix)
        ld      -4(ix), b
        ld      -3(ix), c

        ld      b, -8(ix)
        ld      c, -7(ix)
        ld      -6(ix), b
        ld      -5(ix), c

        xor     a
        ld      -8(ix), a
        ld      -7(ix), a
        pop     af
        jp      .word_shifts

.bit_shifts:
        ld      b, a
        ld      a, b
        or      a
        jp      z, .done

.bit_loop:
        ld      a, -8(ix)
        add     a, a
        ld      -8(ix), a
        ld      a, -7(ix)
        rla
        ld      -7(ix), a
        ld      a, -6(ix)
        rla
        ld      -6(ix), a
        ld      a, -5(ix)
        rla
        ld      -5(ix), a
        ld      a, -4(ix)
        rla
        ld      -4(ix), a
        ld      a, -3(ix)
        rla
        ld      -3(ix), a
        ld      a, -2(ix)
        rla
        ld      -2(ix), a
        ld      a, -1(ix)
        rla
        ld      -1(ix), a
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
