        ; strcpy.s
        ;
        ; libc strcpy implementation for the xcc Z80 libc.
        ; Copies a NUL-terminated source string to the destination and returns
        ; the original destination pointer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcpy
        .globl  __string_copy_cstr

        .area   _CODE

        ; _strcpy
        ; inputs:
        ;   HL = destination
        ;   DE = source
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL
_strcpy::
        ld      b,h                      ; save return pointer in BC
        ld      c,l
        ex      de,hl                    ; HL = source, DE = destination
        call    __string_copy_cstr
        ld      d,b
        ld      e,c
        ret
