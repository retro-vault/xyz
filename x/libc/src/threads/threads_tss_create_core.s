        ;; threads_tss_create_core.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_tss_create_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_tss_create_core
        .globl  __threads_tss_used
        .globl  __threads_tss_values
        .globl  threads_mtx_err_plain

THRD_NOMEM        .equ 3
THRD_SUCCESS      .equ 0
THREADS_TSS_SLOTS .equ 8

        .area   _CODE
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

