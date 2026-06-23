        ;; threads_mtx_timedlock_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_mtx_timedlock_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_mtx_timedlock_core
        .globl  threads_mtx_acquire
        .globl  threads_mtx_err_plain

MTX_COUNT_HI      .equ 3
MTX_COUNT_LO      .equ 2
MTX_FLAGS         .equ 1
MTX_STATE         .equ 0
THRD_SUCCESS      .equ 0
THRD_TIMEDOUT     .equ 4

        .area   _CODE
__threads_mtx_timedlock_core:
        ld      c,#THRD_TIMEDOUT
threads_mtx_acquire::
        ld      a,h
        or      l
        jp      z,threads_mtx_err_plain
        push    ix
        push    hl
        pop     ix
        ld      a,MTX_STATE(ix)
        or      a
        jr      z,threads_mtx_lock_fresh
        ld      a,MTX_FLAGS(ix)
        and     #0x01
        jr      z,threads_mtx_fail_c
        inc     MTX_COUNT_LO(ix)
        jr      nz,threads_mtx_ok
        inc     MTX_COUNT_HI(ix)
threads_mtx_ok:
        ld      de,#THRD_SUCCESS
        pop     ix
        ret
threads_mtx_lock_fresh:
        ld      a,#1
        ld      MTX_STATE(ix),a
        ld      MTX_COUNT_LO(ix),a
        xor     a
        ld      MTX_COUNT_HI(ix),a
        jr      threads_mtx_ok
threads_mtx_fail_c:
        ld      e,c
        ld      d,#0
        pop     ix
        ret

