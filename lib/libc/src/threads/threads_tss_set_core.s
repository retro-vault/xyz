        ;; threads_tss_set_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_tss_set_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_tss_set_core
        .globl  __threads_key_to_slot
        .globl  threads_mtx_err_plain

THRD_SUCCESS      .equ 0

        .area   _CODE
__threads_tss_set_core:
        push    de
        call    __threads_key_to_slot
        jr      c,threads_tss_set_fail
        ld      a,(hl)
        or      a
        jr      z,threads_tss_set_fail
        pop     hl
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        ld      de,#THRD_SUCCESS
        ret
threads_tss_set_fail:
        pop     bc
        jp      threads_mtx_err_plain
