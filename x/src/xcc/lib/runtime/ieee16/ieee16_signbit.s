        ; ieee16_signbit.s
        .module ieee16_signbit
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_signbit

        .area   _CODE
_ieee16_signbit::
        ld      de,#0
        bit     7,h
        ret     z
        inc     de
        ret
