        ; isdigit.s
        ;
        ; libc isdigit implementation for the xcc Z80 libc.
        ; Accepts ASCII decimal digits only.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isdigit
        .optsdcc -mz80 sdcccall(1)


        .globl  _isdigit
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ; _isdigit
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is a decimal digit, else 0
        ; clobbers: AF, BC
_isdigit::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x3930
        call    __ctype_test_interval
        jp      __ctype_return_flag
