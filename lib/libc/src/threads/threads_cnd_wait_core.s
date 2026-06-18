        ;; threads_cnd_wait_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_cnd_wait_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_cnd_wait_core
        .globl  threads_cnd_err

THRD_SUCCESS      .equ 0

        .area   _CODE
__threads_cnd_wait_core:
        ld      a,h
        or      l
        jp      z,threads_cnd_err
        ld      a,d
        or      e
        jp      z,threads_cnd_err
        ld      a,(hl)
        or      a
        jp      z,threads_cnd_err
        ld      (hl),#0
        ld      de,#THRD_SUCCESS
        ret
