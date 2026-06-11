        ;; threads_common.s
        ;;
        ;; Shared single-thread C11 threads fallback for the xcc Z80 libc.
        ;; once flags, mutex bookkeeping, and thread-specific storage are
        ;; implemented directly; true thread creation remains unsupported.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module threads_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_call_once_core
        .globl  __threads_cnd_broadcast_core
        .globl  __threads_cnd_destroy_core
        .globl  __threads_cnd_init_core
        .globl  __threads_cnd_signal_core
        .globl  __threads_cnd_timedwait_core
        .globl  __threads_cnd_wait_core
        .globl  __threads_mtx_destroy_core
        .globl  __threads_mtx_init_core
        .globl  __threads_mtx_lock_core
        .globl  __threads_mtx_timedlock_core
        .globl  __threads_mtx_trylock_core
        .globl  __threads_mtx_unlock_core
        .globl  __threads_thrd_create_core
        .globl  __threads_thrd_current_core
        .globl  __threads_thrd_detach_core
        .globl  __threads_thrd_equal_core
        .globl  __threads_thrd_exit_core
        .globl  __threads_thrd_join_core
        .globl  __threads_thrd_sleep_core
        .globl  __threads_thrd_yield_core
        .globl  __threads_tss_create_core
        .globl  __threads_tss_delete_core
        .globl  __threads_tss_get_core
        .globl  __threads_tss_set_core
        .globl  __sdcc_call_bc
        .globl  __Exit

THREADS_TSS_SLOTS .equ 8
THRD_SUCCESS      .equ 0
THRD_BUSY         .equ 1
THRD_ERROR        .equ 2
THRD_NOMEM        .equ 3
THRD_TIMEDOUT     .equ 4
MTX_STATE         .equ 0
MTX_FLAGS         .equ 1
MTX_COUNT_LO      .equ 2
MTX_COUNT_HI      .equ 3

        .area   _DATA

__threads_tss_used:
        .ds     THREADS_TSS_SLOTS
__threads_tss_values:
        .ds     (THREADS_TSS_SLOTS * 2)

        .area   _CODE

;; Translate key values 1..THREADS_TSS_SLOTS into slot pointers.
;; input:  A = public key
;; output: HL = &used[idx], DE = &value[idx], carry set on invalid key
__threads_key_to_slot:
        or      a
        jr      z,threads_key_invalid
        cp      #(THREADS_TSS_SLOTS + 1)
        jr      nc,threads_key_invalid
        dec     a
        ld      c,a
        ld      b,#0
        ld      hl,#__threads_tss_used
        add     hl,bc
        push    hl
        ld      h,b
        ld      l,c
        add     hl,hl
        ld      de,#__threads_tss_values
        add     hl,de
        ex      de,hl
        pop     hl
        or      a
        ret
threads_key_invalid:
        scf
        ret

__threads_call_once_core:
        ld      a,h
        or      l
        ret     z
        ld      a,(hl)
        or      a
        ret     nz
        ld      (hl),#1
        ld      c,e
        ld      b,d
        ld      a,b
        or      c
        ret     z
        call    __sdcc_call_bc
        ret

__threads_cnd_init_core:
        ld      a,h
        or      l
        jr      z,threads_cnd_err
        ld      (hl),#0
        ld      de,#THRD_SUCCESS
        ret

__threads_cnd_signal_core:
__threads_cnd_broadcast_core:
        ld      a,h
        or      l
        jr      z,threads_cnd_err
        ld      (hl),#1
        ld      de,#THRD_SUCCESS
        ret

__threads_cnd_destroy_core:
        ld      a,h
        or      l
        ret     z
        ld      (hl),#0
        ret

__threads_cnd_wait_core:
        ld      a,h
        or      l
        jr      z,threads_cnd_err
        ld      a,d
        or      e
        jr      z,threads_cnd_err
        ld      a,(hl)
        or      a
        jr      z,threads_cnd_err
        ld      (hl),#0
        ld      de,#THRD_SUCCESS
        ret

__threads_cnd_timedwait_core:
        ld      a,h
        or      l
        jr      z,threads_cnd_err
        ld      a,d
        or      e
        jr      z,threads_cnd_err
        ld      a,(hl)
        or      a
        jr      z,threads_cnd_timeout
        ld      (hl),#0
        ld      de,#THRD_SUCCESS
        ret

threads_cnd_timeout:
        ld      de,#THRD_TIMEDOUT
        ret

threads_cnd_err:
        ld      de,#THRD_ERROR
        ret

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

__threads_mtx_lock_core:
        ld      c,#THRD_ERROR
        jr      threads_mtx_acquire

__threads_mtx_trylock_core:
        ld      c,#THRD_BUSY
        jr      threads_mtx_acquire

__threads_mtx_timedlock_core:
        ld      c,#THRD_TIMEDOUT
threads_mtx_acquire:
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

threads_mtx_err_popix:
        pop     ix
threads_mtx_err_plain:
        ld      de,#THRD_ERROR
        ret

__threads_thrd_create_core:
__threads_thrd_detach_core:
        ld      de,#THRD_ERROR
        ret

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

__threads_thrd_current_core:
        ld      de,#1
        ret

__threads_thrd_equal_core:
        xor     a
        sbc     hl,de
        ld      de,#0
        ret     nz
        inc     de
        ret

__threads_thrd_exit_core:
        jp      __Exit

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

__threads_thrd_yield_core:
        ret

__threads_tss_create_core:
        ld      a,h
        or      l
        jp      z,threads_mtx_err_plain
        ld      d,h
        ld      e,l                      ; DE = out key pointer
        ld      hl,#__threads_tss_used
        ld      b,#0
threads_tss_find:
        ld      a,b
        cp      #THREADS_TSS_SLOTS
        jp      z,threads_tss_nomem
        ld      a,(hl)
        or      a
        jr      z,threads_tss_found
        inc     hl
        inc     b
        jr      threads_tss_find
threads_tss_found:
        ld      (hl),#1
        ld      a,b
        ld      c,a
        ld      b,#0
        push    de
        ld      hl,#__threads_tss_values
        add     hl,bc
        add     hl,bc
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        pop     hl
        ld      a,c
        inc     a
        ld      (hl),a
        ld      de,#THRD_SUCCESS
        ret
threads_tss_nomem:
        ld      de,#THRD_NOMEM
        ret

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
