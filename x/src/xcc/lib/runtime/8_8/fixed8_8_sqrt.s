        ; fixed8_8_sqrt.s
        ;
        ; Square root for non-negative signed 8.8 using Newton iteration:
        ;   g = (g + x / g) / 2
        ; Negative inputs return zero (fixed mode has no NaN).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_sqrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_sqrt
        .globl  _fixed8_8_div
        .globl  _fixed8_8_add
        .globl  _fixed8_8_div2

        .area   _CODE

SQ8_X      .equ -2
SQ8_G      .equ -4
SQ8_COUNT  .equ -5

        ; inputs:  HL = fixed8_8
        ; outputs: DE = sqrt(x)
_fixed8_8_sqrt::
        push    ix
        ld      ix,#0
        add     ix,sp
        bit     7,h
        jr      nz,.zero
        ld      a,h
        or      l
        jr      z,.zero
        ld      b,h
        ld      c,l
        ld      hl,#-5
        add     hl,sp
        ld      sp,hl
        ld      SQ8_X(ix),c
        ld      SQ8_X+1(ix),b

        ; Guess is x for x >= 1.0, otherwise 1.0.
        ld      a,b
        or      a
        jr      nz,.guess_x
        ld      hl,#0x0100
        jr      .store_guess
.guess_x:
        ld      h,b
        ld      l,c
.store_guess:
        ld      SQ8_G(ix),l
        ld      SQ8_G+1(ix),h
        ld      SQ8_COUNT(ix),#8

.loop:
        ld      e,SQ8_G(ix)
        ld      d,SQ8_G+1(ix)
        ld      l,SQ8_X(ix)
        ld      h,SQ8_X+1(ix)
        call    _fixed8_8_div
        ld      l,SQ8_G(ix)
        ld      h,SQ8_G+1(ix)
        call    _fixed8_8_add
        push    de
        pop     hl
        call    _fixed8_8_div2
        ld      SQ8_G(ix),e
        ld      SQ8_G+1(ix),d
        ld      a,SQ8_COUNT(ix)
        dec     a
        ld      SQ8_COUNT(ix),a
        jr      nz,.loop
        ld      sp,ix
        pop     ix
        ret

.zero:
        ld      de,#0
        pop     ix
        ret
