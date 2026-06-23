        ;; threads_tss_used.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_tss_used
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_tss_used
        .globl  __threads_tss_values

THREADS_TSS_SLOTS .equ 8

        .area   _DATA
__threads_tss_used::
        .ds     THREADS_TSS_SLOTS
__threads_tss_values::
        .ds     (THREADS_TSS_SLOTS * 2)

