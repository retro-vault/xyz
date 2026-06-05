        ; islower.s
        ;
        ; libc islower implementation for the xcc Z80 libc.
        ; Accepts ASCII lowercase letters.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module islower
        .optsdcc -mz80 sdcccall(1)


        .globl  _islower
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ; _islower
        ; inputs:  HL = promoted int character value
        ; outputs: DE = 1 when the character is lowercase, else 0
        ; clobbers: AF, BC
_islower::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x7a61
        call    __ctype_test_interval
        jp      __ctype_return_flag
