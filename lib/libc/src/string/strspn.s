        ; strspn.s
        ;
        ; libc strspn implementation for the xcc Z80 libc.
        ; Counts the initial run of characters that all belong to the
        ; accept-set string.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strspn
        .optsdcc -mz80 sdcccall(1)


        .globl  _strspn
        .globl  __string_char_in_set

        .area   _CODE

        ; _strspn
        ; inputs:
        ;   HL = input string
        ;   DE = accept-set string
        ; outputs:
        ;   DE = length of the accepted prefix
        ; clobbers: AF, BC, HL
_strspn::
        ld      b,d                      ; BC = accept-set pointer
        ld      c,e
        ld      de,#0x0000
strspn_loop:
        ld      a,(hl)
        or      a
        ret     z
        push    hl                      ; preserve current input position
        ld      h,b
        ld      l,c
        call    __string_char_in_set
        pop     hl
        ret     nz
        inc     hl
        inc     de
        jr      strspn_loop
