        ;; threads_cnd_err.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_cnd_err
        .optsdcc -mz80 sdcccall(1)

        .globl  threads_cnd_err

THRD_ERROR        .equ 2

        .area   _CODE
threads_cnd_err::
        ld      de,#THRD_ERROR
        ret

