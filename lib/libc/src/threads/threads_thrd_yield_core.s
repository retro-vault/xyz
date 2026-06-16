        ;; threads_thrd_yield_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_yield_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_yield_core

        .area   _CODE
__threads_thrd_yield_core:
        ret

