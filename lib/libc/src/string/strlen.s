        ; strlen.s
        ;
        ; libc strlen implementation for the xcc Z80 libc.
        ; Delegates the byte scan to __string_scan_nul and then subtracts the
        ; original base pointer to obtain the character count.
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
        push    hl                      ; preserve base pointer
        call    __string_scan_nul
        pop     de
        or      a                       ; clear carry before subtracting
        sbc     hl,de                   ; HL = end - start
        ex      de,hl                   ; return the size in DE
        ret
