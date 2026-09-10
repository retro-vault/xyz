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
        ld      a,d
        or      e
        ret     z                       ; zero limit already in DE
        ld      b,d                      ; BC = remaining limit
        ld      c,e
        xor     a
        cpir                            ; consumes at most the specified limit
        ret     nz                      ; exhausted: return original limit
        ex      de,hl                   ; HL = original limit
        sbc     hl,bc                   ; CPIR preserved XOR's clear carry
        dec     hl                      ; exclude the terminating NUL
        ex      de,hl
        ret
