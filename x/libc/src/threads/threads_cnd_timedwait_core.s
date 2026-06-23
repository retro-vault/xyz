        ;; threads_cnd_timedwait_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_cnd_timedwait_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_cnd_timedwait_core
        .globl  threads_cnd_err

THRD_SUCCESS      .equ 0
THRD_TIMEDOUT     .equ 4

        .area   _CODE
__threads_cnd_timedwait_core:
        ld      a,h
        or      l
        jp      z,threads_cnd_err
        ld      a,d
        or      e
        jp      z,threads_cnd_err
        ld      a,(hl)
        or      a
        jr      z,threads_cnd_timeout
        ld      (hl),#0
        ld      de,#THRD_SUCCESS
        ret

threads_cnd_timeout:
        ld      de,#THRD_TIMEDOUT
        ret
