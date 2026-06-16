        ;; float_load_y.s
        ;; Split from fmaxf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module float_load_y
        .optsdcc -mz80 sdcccall(1)

        .globl  __float_load_y

        .area   _CODE
__float_load_y::
        ld      a,7(ix)
        ld      h,a
        ld      a,6(ix)
        ld      l,a
        ld      a,5(ix)
        ld      d,a
        ld      a,4(ix)
        ld      e,a
        ret
