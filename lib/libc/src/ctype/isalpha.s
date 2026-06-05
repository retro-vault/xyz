        ; isalpha.s
        ;
        ; libc isalpha implementation for the xcc Z80 libc.
        ; Accepts ASCII uppercase and lowercase letters.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isalpha
        .optsdcc -mz80 sdcccall(1)


        .globl  _isalpha
        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_test_interval

        .area   _CODE

        ; _isalpha
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is alphabetic, else 0
        ; clobbers: AF, BC
_isalpha::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x5a41
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x7a61
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        jp      __ctype_return_false
