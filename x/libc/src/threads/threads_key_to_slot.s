        ;; threads_key_to_slot.s
        ;; Split from threads_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module threads_key_to_slot
        .optsdcc -mz80 sdcccall(1)

        .globl  __threads_key_to_slot
        .globl  __threads_tss_used
        .globl  __threads_tss_values

THREADS_TSS_SLOTS .equ 8

        .area   _CODE
__threads_key_to_slot::
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

