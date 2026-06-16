        ;; sys_getchar_ptr_storage.s
        ;; Split from sys_getchar.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_getchar_ptr_storage
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_getchar_ptr_storage

        .area   _DATA
__sys_getchar_ptr_storage::
        .dw     0

