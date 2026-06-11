        ; strerror.s
        ;
        ; libc strerror implementation for the xcc Z80 libc.
        ; The current libc only distinguishes the success case from a generic
        ; error string.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strerror
        .optsdcc -mz80 sdcccall(1)


        .globl  _strerror

        .area   _CODE

        ; _strerror
        ; inputs:  HL = error number
        ; outputs: DE = pointer to a static message string
        ; clobbers: AF
_strerror::
        ld      a,h
        or      l
        ld      de,#_strerror_unknown
        ret     nz
        ld      de,#_strerror_none
        ret

        .area   _CONST
_strerror_none:
        .ascii  "No error"
        .db     0
_strerror_unknown:
        .ascii  "Unknown error"
        .db     0
