        ;; ckd_mul_sint.s
        ;; Split from strtox_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ckd_mul_sint
        .optsdcc -mz80 sdcccall(1)

        .globl  __ckd_mul_sint
        .globl  ___mulsint2slong

        .area   _CODE
__ckd_mul_sint::
        push    de
        call    ___mulsint2slong
        pop     bc
        ld      a,h
        rla
        sbc     a,a
        cp      e
        ret     nz
        cp      d
        ret
