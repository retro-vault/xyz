        ; iscntrl.s
        ;
        ; libc iscntrl implementation for the xcc Z80 libc.
        ; Accepts ASCII control characters 0x00..0x1f and 0x7f.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iscntrl
        .optsdcc -mz80 sdcccall(1)


        .globl  _iscntrl
        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_test_interval

        .area   _CODE

        ; _iscntrl
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is a control code, else 0
        ; clobbers: AF, BC
_iscntrl::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x1f00
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        cp      #0x7f
        jp      z,__ctype_return_true
        jp      __ctype_return_false
