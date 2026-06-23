        ; strcspn.s
        ;
        ; libc strcspn implementation for the xcc Z80 libc.
        ; Counts the initial run of characters that are not present in the
        ; reject-set string.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcspn
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcspn

        .globl  __string_char_in_set

        .area   _CODE

        ; _strcspn
        ; inputs:
        ;   HL = input string
        ;   DE = reject-set string
        ; outputs:
        ;   DE = length of the initial accepted prefix
        ; clobbers: AF, BC, HL
_strcspn::
        ld      b,d                      ; BC = reject-set pointer
        ld      c,e
        ld      de,#0x0000
strcspn_loop:
        ld      a,(hl)
        or      a
        ret     z
        push    hl                      ; preserve current input position
        ld      h,b
        ld      l,c
        call    __string_char_in_set
        pop     hl
        ret     z
        inc     hl
        inc     de
        jr      strcspn_loop
