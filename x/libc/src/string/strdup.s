        ; strdup.s
        ;
        ; libc strdup implementation for the xcc Z80 libc.
        ; Computes the string length, allocates length+1 bytes, and copies the
        ; source string including its terminator.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strdup
        .optsdcc -mz80 sdcccall(1)


        .globl  _strdup
        .globl  _strlen
        .globl  _malloc
        .globl  __string_copy_cstr

        .area   _CODE

        ; _strdup
        ; inputs:  HL = source string
        ; outputs: DE = newly allocated copy, or 0 on allocation failure
        ; clobbers: AF, BC, HL
_strdup::
        push    hl                      ; preserve source pointer
        call    _strlen
        inc     de                      ; include the terminating NUL
        ex      de,hl                   ; malloc expects the size in HL
        call    _malloc
        pop     hl
        ld      a,d
        or      e
        ret     z
        push    de                      ; preserve allocated block for return
        call    __string_copy_cstr
        pop     de
        ret
