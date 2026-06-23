        ; fixed16_16_sqrt.s
        ;
        ; Square root for non-negative signed 16.16 using Newton iteration:
        ;   g = (g + x / g) / 2
        ; Negative inputs return zero (fixed mode has no NaN).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_sqrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_sqrt
        .globl  _fixed16_16_div
        .globl  _fixed16_16_add
        .globl  _fixed16_16_div2

        .area   _CODE

SQ16_X     .equ -4
SQ16_G     .equ -8
SQ16_COUNT .equ -9

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE:HL = sqrt(x)
_fixed16_16_sqrt::
        push    ix
        ld      ix,#0
        add     ix,sp
        bit     7,h
        jp      nz,.zero
        ld      a,h
        or      l
        or      d
        or      e
        jp      z,.zero
        ld      b,h
        ld      c,l
        ld      hl,#-9
        add     hl,sp
        ld      sp,hl
        ld      SQ16_X(ix),e
        ld      SQ16_X+1(ix),d
        ld      SQ16_X+2(ix),c
        ld      SQ16_X+3(ix),b

        ; Guess is x for x >= 1.0, otherwise 1.0.
        ld      a,b
        or      c
        jr      nz,.guess_x
        ld      de,#0
        ld      hl,#1
        jr      .store_guess
.guess_x:
        ld      e,SQ16_X(ix)
        ld      d,SQ16_X+1(ix)
        ld      l,SQ16_X+2(ix)
        ld      h,SQ16_X+3(ix)
.store_guess:
        ld      SQ16_G(ix),e
        ld      SQ16_G+1(ix),d
        ld      SQ16_G+2(ix),l
        ld      SQ16_G+3(ix),h
        ld      SQ16_COUNT(ix),#8

.loop:
        ; x / g
        ld      l,SQ16_G+2(ix)
        ld      h,SQ16_G+3(ix)
        push    hl
        ld      l,SQ16_G(ix)
        ld      h,SQ16_G+1(ix)
        push    hl
        ld      e,SQ16_X(ix)
        ld      d,SQ16_X+1(ix)
        ld      l,SQ16_X+2(ix)
        ld      h,SQ16_X+3(ix)
        call    _fixed16_16_div
        pop     bc
        pop     bc

        ; (g + x/g) / 2
        ld      l,SQ16_G+2(ix)
        ld      h,SQ16_G+3(ix)
        push    hl
        ld      l,SQ16_G(ix)
        ld      h,SQ16_G+1(ix)
        push    hl
        call    _fixed16_16_add
        pop     bc
        pop     bc
        call    _fixed16_16_div2
        ld      SQ16_G(ix),e
        ld      SQ16_G+1(ix),d
        ld      SQ16_G+2(ix),l
        ld      SQ16_G+3(ix),h
        ld      a,SQ16_COUNT(ix)
        dec     a
        ld      SQ16_COUNT(ix),a
        jr      nz,.loop
        ld      sp,ix
        pop     ix
        ret

.zero:
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret
