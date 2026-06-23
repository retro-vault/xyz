        ;; threads_cnd_signal_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_cnd_signal_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_cnd_broadcast_core
        .globl  __threads_cnd_signal_core
        .globl  threads_cnd_err

THRD_SUCCESS      .equ 0

        .area   _CODE
__threads_cnd_signal_core:
__threads_cnd_broadcast_core:
        ld      a,h
        or      l
        jp      z,threads_cnd_err
        ld      (hl),#1
        ld      de,#THRD_SUCCESS
        ret
