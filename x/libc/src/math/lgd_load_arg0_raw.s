        ;; lgd_load_arg0_raw.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lgd_load_arg0_raw
        .optsdcc -mz80 sdcccall(1)

        .globl  __lgd_load_arg0_raw

        .area   _CODE
__lgd_load_arg0_raw::
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        exx
        ld      a,8(ix)
        ld      e,a
        ld      a,9(ix)
        ld      d,a
        ld      a,10(ix)
        ld      l,a
        ld      a,11(ix)
        ld      h,a
        exx
        ret

