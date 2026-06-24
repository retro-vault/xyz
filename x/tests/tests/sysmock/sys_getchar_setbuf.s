        ;; sys_getchar_setbuf.s
        ;; Split from sys_getchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_getchar_setbuf
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_getchar_setbuf
        .globl  __sys_getchar_setbuf
        .globl  __sys_getchar_ptr_storage

        .area   _CODE
__sys_getchar_setbuf:
___sys_getchar_setbuf::
        ld      (__sys_getchar_ptr_storage),hl
        ex      de,hl
        ret

