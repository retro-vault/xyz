        ;; threads_mtx_unlock_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_unlock_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_unlock_core
        .globl  threads_mtx_err_plain
        .globl  threads_mtx_err_popix

MTX_COUNT_HI      .equ 3
MTX_COUNT_LO      .equ 2
MTX_STATE         .equ 0
THRD_SUCCESS      .equ 0

        .area   _CODE
__threads_mtx_unlock_core:
        ld      a,h
        or      l
        jp      z,threads_mtx_err_plain
        push    ix
        push    hl
        pop     ix
        ld      a,MTX_STATE(ix)
        or      a
        jp      z,threads_mtx_err_popix
        ld      a,MTX_COUNT_HI(ix)
        or      a
        jr      nz,threads_mtx_dec
        ld      a,MTX_COUNT_LO(ix)
        cp      #1
        jr      nz,threads_mtx_dec
        xor     a
        ld      MTX_STATE(ix),a
        ld      MTX_COUNT_LO(ix),a
        ld      MTX_COUNT_HI(ix),a
        ld      de,#THRD_SUCCESS
        pop     ix
        ret
threads_mtx_dec:
        ld      a,MTX_COUNT_LO(ix)
        or      a
        jr      nz,threads_mtx_dec_low
        dec     MTX_COUNT_HI(ix)
threads_mtx_dec_low:
        dec     MTX_COUNT_LO(ix)
        ld      de,#THRD_SUCCESS
        pop     ix
        ret

