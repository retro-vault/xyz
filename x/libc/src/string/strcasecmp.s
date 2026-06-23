        ; strcasecmp.s
        ;
        ; libc strcasecmp implementation for the xcc Z80 libc.
        ; Case-insensitive (ASCII) comparison of two NUL-terminated strings,
        ; returning a tri-state result (POSIX extension).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcasecmp
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcasecmp
        .globl  __string_fold_lower

        .area   _CODE

        ; _strcasecmp
        ; inputs:  HL = first string, DE = second string
        ; outputs: DE = -1, 0, or 1
        ; clobbers: AF, BC, HL
_strcasecmp::
strcasecmp_loop:
        ld      a,(de)
        call    __string_fold_lower     ; B = tolower(second char)
        ld      b,a
        ld      a,(hl)
        call    __string_fold_lower     ; A = tolower(first char)
        cp      b
        jr      c,strcasecmp_lt
        jr      nz,strcasecmp_gt
        ; equal folded bytes; stop at end of string
        or      a
        jr      z,strcasecmp_eq
        inc     hl
        inc     de
        jr      strcasecmp_loop
strcasecmp_lt:
        ld      de,#0xffff              ; -1
        ret
strcasecmp_gt:
        ld      de,#0x0001
        ret
strcasecmp_eq:
        ld      de,#0x0000
        ret
