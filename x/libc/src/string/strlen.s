        ; strlen.s
        ;
        ; libc strlen implementation for the xcc Z80 libc.
        ; The shared scanner leaves BC = 65534 - length and carry clear,
        ; so subtracting that count produces the length without a saved base.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strlen
        .optsdcc -mz80 sdcccall(1)


        .globl  _strlen
        .globl  __string_scan_nul

        .area   _CODE

        ; _strlen
        ; inputs:  HL = string pointer
        ; outputs: DE = string length, excluding the terminating NUL
        ; clobbers: AF, BC, HL
_strlen::
        call    __string_scan_nul
        ld      hl,#0xfffe
        sbc     hl,bc
        ex      de,hl
        ret
