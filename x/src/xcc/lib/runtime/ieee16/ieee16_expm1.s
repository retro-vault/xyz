        ; ieee16_expm1.s
        .module ieee16_expm1
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_expm1
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _expm1f

        .area   _CODE
_ieee16_expm1::
        call    ___fh2fs
        call    _expm1f
        jp      ___fs2fh
