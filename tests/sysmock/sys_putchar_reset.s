        ;; sys_putchar_reset.s
        ;; Split from sys_putchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_putchar_reset
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_putchar_reset
        .globl  __sys_putchar_reset
        .globl  __sys_putchar_buffer_storage
        .globl  __sys_putchar_count_storage

        .area   _CODE
__sys_putchar_reset:
___sys_putchar_reset::
        xor     a
        ld      (__sys_putchar_count_storage),a
        ld      (__sys_putchar_count_storage + 1),a
        ld      (__sys_putchar_buffer_storage),a
        ld      de,#0x0000
        ret

