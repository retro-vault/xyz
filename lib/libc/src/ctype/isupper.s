        ; isupper.s
        ;
        ; libc isupper implementation for the xcc Z80 libc.
        ; Accepts ASCII uppercase letters.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isupper
        .optsdcc -mz80 sdcccall(1)


        .globl  _isupper
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ; _isupper
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is uppercase, else 0
        ; clobbers: AF, BC
_isupper::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x5a41
        call    __ctype_test_interval
        jp      __ctype_return_flag
