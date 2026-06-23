        ;; sys_putchar_getchar.s
        ;; Split from sys_putchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_putchar_getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_putchar_getchar
        .globl  __sys_putchar_getchar
        .globl  __sys_putchar_buffer_storage
        .globl  __sys_putchar_count_storage

        .area   _CODE
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

