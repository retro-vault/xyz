        ;; ckd_sub_sint.s
        ;; Split from strtox_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ckd_sub_sint
        .optsdcc -mz80 sdcccall(1)

        .globl  __ckd_sub_sint

        .area   _CODE
__ckd_sub_sint::
        or      a
        sbc     hl,de
        ld      a,h
        xor     b
        ld      c,a
        ld      a,h
        xor     d
        and     c
        and     #0x80
        ret

