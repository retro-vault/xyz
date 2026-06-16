        ;; threads_thrd_sleep_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_sleep_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_sleep_core

        .area   _CODE
__threads_thrd_sleep_core:
        ld      a,d
        or      e
        jr      z,threads_sleep_done
        ex      de,hl
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
threads_sleep_done:
        ld      de,#0
        ret

