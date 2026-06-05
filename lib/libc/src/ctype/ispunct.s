        ; ispunct.s
        ;
        ; libc ispunct implementation for the xcc Z80 libc.
        ; Accepts ASCII punctuation ranges and rejects alnum/space/control.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ispunct
        .optsdcc -mz80 sdcccall(1)


        .globl  _ispunct
        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_test_interval

        .area   _CODE

        ; _ispunct
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is punctuation, else 0
        ; clobbers: AF, BC
_ispunct::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x2f21
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x403a
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x605b
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x7e7b
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        jp      __ctype_return_false
