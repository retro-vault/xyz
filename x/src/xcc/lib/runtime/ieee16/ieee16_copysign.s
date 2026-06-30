        ; ieee16_copysign.s
        .module ieee16_copysign
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_copysign

        .area   _CODE
_ieee16_copysign::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        and     #0x7f
        ld      d,a
        ld      e,l
        ld      a,5(ix)
        and     #0x80
        or      d
        ld      d,a
        pop     ix
        ret
