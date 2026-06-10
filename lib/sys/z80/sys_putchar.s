        ; sys_putchar.s  (sys backend: generic z80)
        ;
        ; Generic bare-metal Z80 builds have no standard console device. The
        ; hook reports success and discards the byte so stdio callers still
        ; link cleanly.
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
        ld      e,l
        ld      d,#0x00
        ret
