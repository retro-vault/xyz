        ;; threads_thrd_join_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_thrd_join_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_thrd_join_core

THRD_ERROR        .equ 2

        .area   _CODE
__threads_thrd_join_core:
        ld      a,d
        or      e
        jr      z,threads_join_no_store
        ex      de,hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
threads_join_no_store:
        ld      de,#THRD_ERROR
        ret

