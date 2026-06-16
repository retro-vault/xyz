        ;; threads_mtx_trylock_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_trylock_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_trylock_core
        .globl  threads_mtx_acquire

THRD_BUSY         .equ 1

        .area   _CODE
__threads_mtx_trylock_core:
        ld      c,#THRD_BUSY
        jr      threads_mtx_acquire

