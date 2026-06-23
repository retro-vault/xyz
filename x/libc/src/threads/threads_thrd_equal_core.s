        ;; threads_thrd_equal_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_equal_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_equal_core

        .area   _CODE
__threads_thrd_equal_core:
        xor     a
        sbc     hl,de
        ld      de,#0
        ret     nz
        inc     de
        ret

