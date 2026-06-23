        ;; threads_tss_get_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_tss_get_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_tss_get_core
        .globl  __threads_key_to_slot

        .area   _CODE
__threads_tss_get_core:
        call    __threads_key_to_slot
        jp      c,threads_tss_get_fail
        ld      a,(hl)
        or      a
        jp      z,threads_tss_get_fail
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a
        ex      de,hl
        ret
threads_tss_get_fail:
        ld      de,#0
        ret

