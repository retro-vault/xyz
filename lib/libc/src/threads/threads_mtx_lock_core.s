        ;; threads_mtx_lock_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_lock_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_lock_core
        .globl  threads_mtx_acquire

THRD_ERROR        .equ 2

        .area   _CODE
__threads_mtx_lock_core:
        ld      c,#THRD_ERROR
        jp      threads_mtx_acquire
