        ; isprint.s
        ;
        ; libc isprint implementation for the xcc Z80 libc.
        ; Accepts printable ASCII characters including space.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isprint
        .optsdcc -mz80 sdcccall(1)


        .globl  _isprint
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ; _isprint
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is printable, else 0
        ; clobbers: AF, BC
_isprint::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x7e20
        call    __ctype_test_interval
        jp      __ctype_return_flag
