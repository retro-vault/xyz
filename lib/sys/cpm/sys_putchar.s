        ; sys_putchar.s  (sys backend: CP/M)
        ;
        ; Emit one byte through the BDOS console-output call.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sys_putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_putchar
        .globl  ___sys_putchar

        .area   _CODE

__sys_putchar:
___sys_putchar::
        ld      a,l
        push    af
        ld      e,a
        ld      c,#2
        call    5
        pop     af
        ld      e,a
        ld      d,#0x00
        ret
