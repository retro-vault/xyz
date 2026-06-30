        ; ieee16_nan.s
        .module ieee16_nan
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_nan

        .area   _CODE
_ieee16_nan::
        ld      de,#0x7e00
        ret
