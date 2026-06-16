        ;; threads_mtx_init_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_init_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_init_core
        .globl  threads_mtx_err_plain

MTX_COUNT_HI      .equ 3
MTX_COUNT_LO      .equ 2
MTX_FLAGS         .equ 1
MTX_STATE         .equ 0
THRD_SUCCESS      .equ 0

        .area   _CODE
__threads_mtx_init_core:
        ld      a,h
        or      l
        jp      z,threads_mtx_err_plain
        push    ix
        push    hl
        pop     ix
        xor     a
        ld      MTX_STATE(ix),a
        ld      a,e
        and     #0x03
        ld      MTX_FLAGS(ix),a
        xor     a
        ld      MTX_COUNT_LO(ix),a
        ld      MTX_COUNT_HI(ix),a
        ld      de,#THRD_SUCCESS
        pop     ix
        ret

