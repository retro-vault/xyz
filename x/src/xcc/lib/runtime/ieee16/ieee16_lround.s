        ; ieee16_lround.s
        .module ieee16_lround
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_lround
        .globl  ___fh2fs
        .globl  _lroundf

        .area   _CODE
_ieee16_lround::
        call    ___fh2fs
        jp      _lroundf
