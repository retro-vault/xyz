        ; strnlen.s
        ;
        ; libc strnlen implementation for the xcc Z80 libc.
        ; Counts characters until either the first NUL byte or the caller's
        ; upper bound is reached.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strnlen
        .optsdcc -mz80 sdcccall(1)


        .globl  _strnlen

        .area   _CODE

        ; _strnlen
        ; inputs:
        ;   HL = string pointer
        ;   DE = maximum character count
        ; outputs:
        ;   DE = bounded string length
        ; clobbers: AF, BC, HL
_strnlen::
        ld      b,d                      ; BC = remaining limit
        ld      c,e
        ld      de,#0x0000
strnlen_loop:
        ld      a,b
        or      c
        ret     z
        ld      a,(hl)
        or      a
        ret     z
        inc     hl
        dec     bc
        inc     de
        jr      strnlen_loop
