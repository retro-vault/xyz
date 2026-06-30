        ; ieee16_trunc.s
        .module ieee16_trunc
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_trunc
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _truncf

        .area   _CODE
_ieee16_trunc::
        call    ___fh2fs
        call    _truncf
        jp      ___fs2fh
