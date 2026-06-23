        ; strpbrk.s
        ;
        ; libc strpbrk implementation for the xcc Z80 libc.
        ; Returns the first character in the input string that belongs to the
        ; accept-set string.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strpbrk
        .optsdcc -mz80 sdcccall(1)


        .globl  _strpbrk
        .globl  __string_char_in_set
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

        ; _strpbrk
        ; inputs:
        ;   HL = input string
        ;   DE = accept-set string
        ; outputs:
        ;   DE = pointer to the first accepted byte, or 0
        ; clobbers: AF, BC, HL
_strpbrk::
        ld      b,d                      ; BC = accept-set pointer
        ld      c,e
strpbrk_loop:
        ld      a,(hl)
        or      a
        jr      z,strpbrk_not_found
        push    hl                      ; keep current candidate position
        ld      h,b
        ld      l,c
        call    __string_char_in_set
        pop     hl
        jr      z,strpbrk_found
        inc     hl
        jr      strpbrk_loop
strpbrk_found:
        jp      __string_return_hl
strpbrk_not_found:
        jp      __string_return_zero
