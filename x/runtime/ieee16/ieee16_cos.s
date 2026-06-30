        ; ieee16_cos.s
        .module ieee16_cos
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_cos
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _cosf

        .area   _CODE
_ieee16_cos::
        call    ___fh2fs
        call    _cosf
        jp      ___fs2fh
