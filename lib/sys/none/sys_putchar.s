        ; sys_putchar.s  (sys backend: none / bare metal)
        ;
        ; Output hook for the output-only stdio implementation. The "none"
        ; backend has no console device, so it captures bytes in a small RAM
        ; buffer for debugger inspection and automated tests.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sys_putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_putchar
        .globl  ___sys_putchar
        .globl  __sys_putchar_reset
        .globl  ___sys_putchar_reset
        .globl  __sys_putchar_getcount
        .globl  ___sys_putchar_getcount
        .globl  __sys_putchar_getchar
        .globl  ___sys_putchar_getchar

        .area   _DATA
__sys_putchar_count_storage:
        .dw     0
__sys_putchar_buffer_storage:
        .ds     256

        .area   _CODE

__sys_putchar_reset:
___sys_putchar_reset::
        xor     a
        ld      (__sys_putchar_count_storage),a
        ld      (__sys_putchar_count_storage + 1),a
        ld      (__sys_putchar_buffer_storage),a
        ld      de,#0x0000
        ret

__sys_putchar_getcount:
___sys_putchar_getcount::
        ld      hl,(__sys_putchar_count_storage)
        ex      de,hl
        ret

__sys_putchar_getchar:
___sys_putchar_getchar::
        ld      a,h
        or      a
        jr      nz,putchar_get_fail
        ld      a,l
        ld      c,a
        ld      hl,(__sys_putchar_count_storage)
        ld      a,h
        or      a
        jr      nz,putchar_get_fail
        ld      a,c
        cp      l
        jr      nc,putchar_get_fail
        ld      hl,#__sys_putchar_buffer_storage
        ld      e,c
        ld      d,#0x00
        add     hl,de
        ld      e,(hl)
        ld      d,#0x00
        ret
putchar_get_fail:
        ld      de,#0xffff
        ret

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
