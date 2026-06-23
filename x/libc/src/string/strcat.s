        ; strcat.s
        ;
        ; libc strcat implementation for the xcc Z80 libc.
        ; Locates the destination terminator, then copies the source string
        ; including its trailing NUL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcat
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcat
        .globl  __string_scan_nul
        .globl  __string_copy_cstr

        .area   _CODE

        ; _strcat
        ; inputs:
        ;   HL = destination string
        ;   DE = source string
        ; outputs:
        ;   DE = original destination pointer
        ; clobbers: AF, BC, HL
_strcat::
        ld      b,h                      ; keep original destination in BC
        ld      c,l
        push    de                       ; preserve source pointer
        call    __string_scan_nul
        pop     de
        ex      de,hl                    ; HL = source, DE = destination end
        call    __string_copy_cstr
        ld      d,b
        ld      e,c
        ret
