        ;; threads_mtx_err_popix.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_err_popix
        .optsdcc -mz80 sdcccall(1)

        .globl  threads_mtx_err_plain
        .globl  threads_mtx_err_popix

THRD_ERROR        .equ 2

        .area   _CODE
threads_mtx_err_popix::
        pop     ix
threads_mtx_err_plain::
        ld      de,#THRD_ERROR
        ret

