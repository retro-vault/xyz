        ;; sys_putchar_getcount.s
        ;; Split from sys_putchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_putchar_getcount
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_putchar_getcount
        .globl  __sys_putchar_getcount
        .globl  __sys_putchar_count_storage

        .area   _CODE
__sys_putchar_getcount:
___sys_putchar_getcount::
        ld      hl,(__sys_putchar_count_storage)
        ex      de,hl
        ret

