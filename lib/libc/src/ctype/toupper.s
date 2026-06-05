        ; toupper.s
        ;
        ; libc toupper implementation for the xcc Z80 libc.
        ; Converts ASCII lowercase letters to uppercase and leaves everything
        ; else unchanged.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module toupper
        .optsdcc -mz80 sdcccall(1)


        .globl  _toupper
        .globl  __ctype_return_hl
        .globl  __ctype_test_interval

        .area   _CODE

        ; _toupper
        ; inputs:  HL = promoted int character value
        ; outputs: DE = converted character value
        ; clobbers: AF, BC
_toupper::
        ld      a,h
        or      a
        jp      nz,__ctype_return_hl
        ld      a,l
        ld      de,#0x7a61
        call    __ctype_test_interval
        jr      nz,toupper_return_a
        sub     #0x20
toupper_return_a:
        ld      e,a
        ld      d,#0x00
        ret
