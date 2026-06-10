        ; sys_putchar.s  (sys backend: ZX Spectrum ROM image)
        ;
        ; Emit one byte through the ROM character-output routine.
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
        rst     0x10
        ld      e,a
        ld      d,#0x00
        ret
