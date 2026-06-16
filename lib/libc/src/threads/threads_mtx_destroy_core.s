        ;; threads_mtx_destroy_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_destroy_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_destroy_core

MTX_COUNT_HI      .equ 3
MTX_COUNT_LO      .equ 2
MTX_FLAGS         .equ 1
MTX_STATE         .equ 0

        .area   _CODE
__threads_mtx_destroy_core:
        ld      a,h
        or      l
        ret     z
        push    ix
        push    hl
        pop     ix
        xor     a
        ld      MTX_STATE(ix),a
        ld      MTX_FLAGS(ix),a
        ld      MTX_COUNT_LO(ix),a
        ld      MTX_COUNT_HI(ix),a
        pop     ix
        ret

