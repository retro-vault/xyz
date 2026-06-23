        ;; totalorderf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module totalorderf
        .optsdcc -mz80 sdcccall(1)

        .globl  _totalorderf
        .globl  __float_cmp_xy

        .area   _CODE
_totalorderf::
        ; compare total order (bitwise for basic)
        call    __float_cmp_xy
        ld      de,#0
        or      a
        ret     z
        ld      de,#1
        ret

