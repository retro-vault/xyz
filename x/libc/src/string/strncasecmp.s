        ; strncasecmp.s
        ;
        ; libc strncasecmp implementation for the xcc Z80 libc.
        ; Case-insensitive (ASCII) comparison of at most n bytes (POSIX).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strncasecmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _strncasecmp
        .globl  __string_fold_lower

        .area   _CODE

        ; _strncasecmp
        ; inputs:  HL = first string, DE = second string, 4(ix)..5(ix) = n
        ; outputs: DE = -1, 0, or 1
        ; clobbers: AF, BC, HL, IX
_strncasecmp::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
strncasecmp_loop:
        ld      a,b
        or      c
        jr      z,strncasecmp_eq        ; n exhausted: strings equal so far
        dec     bc
        ld      a,(de)
        call    __string_fold_lower
        push    bc
        ld      b,a
        ld      a,(hl)
        call    __string_fold_lower
        cp      b
        pop     bc
        jr      c,strncasecmp_lt
        jr      nz,strncasecmp_gt
        or      a
        jr      z,strncasecmp_eq        ; both hit NUL together
        inc     hl
        inc     de
        jr      strncasecmp_loop
strncasecmp_lt:
        pop     ix
        ld      de,#0xffff
        ret
strncasecmp_gt:
        pop     ix
        ld      de,#0x0001
        ret
strncasecmp_eq:
        pop     ix
        ld      de,#0x0000
        ret
