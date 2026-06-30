        ; ieee16_asinh.s
        .module ieee16_asinh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_asinh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _asinhf

        .area   _CODE
_ieee16_asinh::
        call    ___fh2fs
        call    _asinhf
        jp      ___fs2fh
