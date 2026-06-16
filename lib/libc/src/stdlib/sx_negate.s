        ;; sx_negate.s
        ;; Split from strtox_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sx_negate
        .optsdcc -mz80 sdcccall(1)

        .globl  __ckd_add_sint
        .globl  __sx_negate
        .globl  __ckd_mul_sint
        .globl  __ckd_sub_sint

        .area   _CODE
__sx_negate::
        ld      b,#8
sxn_cpl:
        ld      a,(hl)
        cpl
        ld      (hl),a
        inc     hl
        djnz    sxn_cpl
        ld      bc,#-8
        add     hl,bc
        ld      b,#8
        scf
sxn_inc:
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        djnz    sxn_inc
        ret

        ;; C23 checked-int helpers kept here for now because the header already
        ;; targets these symbols. They do not depend on the parser locals.

        .globl  __ckd_add_sint
        .globl  __ckd_sub_sint
        .globl  __ckd_mul_sint

__ckd_add_sint::
        add     hl,de
        ld      a,h
        xor     b
        ld      c,a
        ld      a,h
        xor     d
        and     c
        and     #0x80
        ret

