        ; strncmp.s
        ;
        ; libc strncmp implementation for the xcc Z80 libc.
        ; Stops at the first mismatch, the first NUL byte, or after consuming
        ; the caller-supplied maximum count.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strncmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _strncmp
        .globl  __string_compare_result

        .area   _CODE

        ; _strncmp
        ; inputs:
        ;   HL         = left-hand string
        ;   DE         = right-hand string
        ;   4(ix)..5(ix) = maximum character count
        ; outputs:
        ;   DE = negative / zero / positive comparison result
        ; clobbers: AF, BC, HL, IX
_strncmp::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        ex      de,hl                   ; DE = lhs, HL = rhs
strncmp_loop:
        ld      a,b
        or      c
        jr      z,strncmp_equal
        ld      a,(de)
        cp      (hl)
        jr      z,strncmp_same
        call    __string_compare_result
        pop     ix
        ret
strncmp_same:
        or      a
        jr      z,strncmp_equal
        inc     de
        inc     hl
        dec     bc
        jr      strncmp_loop
strncmp_equal:
        ld      de,#0x0000
        pop     ix
        ret
