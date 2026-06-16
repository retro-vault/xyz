        ; sys_putchar.s  (sys backend: sim (in-RAM simulator))
        ;
        ; Output hook for the output-only stdio implementation. The "none"
        ; backend has no console device, so it captures bytes in a small RAM
        ; buffer for debugger inspection and automated tests.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih



        .module sys_putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_putchar
        .globl  __sys_putchar
        .globl  __sys_putchar_buffer_storage
        .globl  __sys_putchar_count_storage

        .area   _CODE
__sys_putchar:
___sys_putchar::
        ld      c,l
        ld      hl,(__sys_putchar_count_storage)
        ld      a,h
        or      a
        jr      nz,putchar_return
        ld      a,l
        cp      #255
        jr      nc,putchar_return
        ld      de,#__sys_putchar_buffer_storage
        add     hl,de
        ld      a,c
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        ld      hl,(__sys_putchar_count_storage)
        inc     hl
        ld      (__sys_putchar_count_storage),hl
putchar_return:
        ld      e,c
        ld      d,#0x00
        ret
