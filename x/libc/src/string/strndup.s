        ; strndup.s
        ;
        ; libc strndup implementation for the xcc Z80 libc.
        ; Measures the bounded source length, allocates length+1 bytes, copies
        ; exactly that many bytes, and appends a terminating NUL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strndup
        .optsdcc -mz80 sdcccall(1)


        .globl  _strndup
        .globl  _strnlen
        .globl  _malloc

        .area   _CODE

        ; _strndup
        ; inputs:
        ;   HL = source string
        ;   DE = maximum byte count
        ; outputs:
        ;   DE = newly allocated bounded copy, or 0 on allocation failure
        ; clobbers: AF, BC, HL
_strndup::
        push    hl                      ; preserve source pointer
        call    _strnlen
        push    de                      ; keep measured length in BC later
        inc     de                      ; +1 for the NUL terminator
        ex      de,hl                   ; malloc expects the size in HL
        call    _malloc
        pop     bc                      ; BC = bounded source length
        pop     hl
        ld      a,d
        or      e
        ret     z
        push    de                      ; preserve allocated block for return
        ld      a,b
        or      c
        jr      z,strndup_term
        ldir                            ; copy only the measured prefix
strndup_term:
        xor     a
        ld      (de),a
        pop     de
        ret
