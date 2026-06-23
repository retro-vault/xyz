        ; strstr.s
        ;
        ; libc strstr implementation for the xcc Z80 libc.
        ; Uses a straightforward nested scan: try each haystack position as a
        ; candidate start and then walk the needle until a mismatch or match.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strstr
        .optsdcc -mz80 sdcccall(1)


        .globl  _strstr
        .globl  __string_return_zero
        .globl  __string_return_hl

        .area   _CODE

        ; _strstr
        ; inputs:
        ;   HL = haystack string
        ;   DE = needle string
        ; outputs:
        ;   DE = pointer to the first match, or 0
        ; clobbers: AF, BC, HL
_strstr::
        ld      a,(de)
        or      a                       ; empty needle matches at the start
        jp      z,__string_return_hl
strstr_outer:
        ld      a,(hl)
        or      a
        jr      z,strstr_not_found
        push    hl                      ; save haystack candidate
        push    de                      ; save needle base
strstr_inner:
        ld      a,(de)
        or      a
        jr      z,strstr_found
        ld      c,a
        ld      a,(hl)
        or      a
        jr      z,strstr_mismatch
        cp      c
        jr      nz,strstr_mismatch
        inc     hl
        inc     de
        jr      strstr_inner
strstr_found:
        pop     de
        pop     de
        ret
strstr_mismatch:
        pop     de
        pop     hl
        inc     hl
        jr      strstr_outer
strstr_not_found:
        jp      __string_return_zero
