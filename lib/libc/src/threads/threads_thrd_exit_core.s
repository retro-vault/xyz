        ;; threads_thrd_exit_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_exit_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_exit_core
        .globl  __Exit

        .area   _CODE
__threads_thrd_exit_core:
        jp      __Exit

