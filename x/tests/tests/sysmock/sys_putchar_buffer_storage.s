        ;; sys_putchar_buffer_storage.s
        ;; Split from sys_putchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_putchar_buffer_storage
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_putchar_buffer_storage

        .area   _DATA
__sys_putchar_buffer_storage::
        .ds     256

