        ; ieee16_erf.s
        .module ieee16_erf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_erf
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _erff

        .area   _CODE
_ieee16_erf::
        call    ___fh2fs
        call    _erff
        jp      ___fs2fh
