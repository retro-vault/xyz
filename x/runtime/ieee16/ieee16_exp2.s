        ; ieee16_exp2.s
        .module ieee16_exp2
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_exp2
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _exp2f

        .area   _CODE
_ieee16_exp2::
        call    ___fh2fs
        call    _exp2f
        jp      ___fs2fh
