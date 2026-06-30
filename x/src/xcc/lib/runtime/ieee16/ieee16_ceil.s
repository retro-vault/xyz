        ; ieee16_ceil.s
        .module ieee16_ceil
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_ceil
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _ceilf

        .area   _CODE
_ieee16_ceil::
        call    ___fh2fs
        call    _ceilf
        jp      ___fs2fh
