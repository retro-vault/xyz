        ;; threads_thrd_create_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_create_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_create_core
        .globl  __threads_thrd_detach_core

THRD_ERROR        .equ 2

        .area   _CODE
__threads_thrd_create_core:
__threads_thrd_detach_core:
        ld      de,#THRD_ERROR
        ret

