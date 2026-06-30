        ; ieee16_abs.s
        .module ieee16_abs
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_abs

        .area   _CODE
_ieee16_abs::
        res     7,h
        ld      d,h
        ld      e,l
        ret
