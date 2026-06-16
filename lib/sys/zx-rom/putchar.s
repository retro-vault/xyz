        ; sys_putchar.s  (sys backend: ZX Spectrum ROM image)
        ;
        ; Emit one byte through the ROM character-output routine.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putchar
        .globl  _putchar

        .area   _CODE

_putchar:
_putchar::
        ld      a,l
        rst     0x10
        ld      e,a
        ld      d,#0x00
        ret
