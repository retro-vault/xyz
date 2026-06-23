        ;; threads_tss_delete_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_tss_delete_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_tss_delete_core
        .globl  __threads_key_to_slot

        .area   _CODE
__threads_tss_delete_core:
        call    __threads_key_to_slot
        jp      c,threads_tss_delete_done
        xor     a
        ld      (hl),a
        ex      de,hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
threads_tss_delete_done:
        ret

