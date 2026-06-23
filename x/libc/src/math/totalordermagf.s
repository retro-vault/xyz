        ;; totalordermagf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module totalordermagf
        .optsdcc -mz80 sdcccall(1)

        .globl  _totalordermagf
        .globl  __float_cmp_xy

        .area   _CODE
_totalordermagf::
        ; mag
        res     7,h
        res     7,b   ; assume y in bc or per
        call    __float_cmp_xy
        ld      de,#0
        or      a
        ret     z
        ld      de,#1
        ret

        ;; float nextdownf(float x)
