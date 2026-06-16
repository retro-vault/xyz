        ;; sys_getchar_reset.s
        ;; Split from sys_getchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_getchar_reset
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_getchar_reset
        .globl  __sys_getchar_reset
        .globl  __sys_getchar_ptr_storage

        .area   _CODE
__sys_getchar_reset:
___sys_getchar_reset::
        xor     a
        ld      (__sys_getchar_ptr_storage),a
        ld      (__sys_getchar_ptr_storage + 1),a
        ld      de,#0x0000
        ret

