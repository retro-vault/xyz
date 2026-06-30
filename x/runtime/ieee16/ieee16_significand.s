        ; ieee16_significand.s
        .module ieee16_significand
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_significand
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _significandf

        .area   _CODE
_ieee16_significand::
        call    ___fh2fs
        call    _significandf
        jp      ___fs2fh
