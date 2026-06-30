        ; ieee16_lrint.s
        .module ieee16_lrint
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_lrint
        .globl  ___fh2fs
        .globl  _lrintf

        .area   _CODE
_ieee16_lrint::
        call    ___fh2fs
        jp      _lrintf
