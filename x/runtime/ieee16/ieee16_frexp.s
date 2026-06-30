        ; ieee16_frexp.s
        .module ieee16_frexp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_frexp
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _frexpf

        .area   _CODE
_ieee16_frexp::
        call    ___fh2fs
        call    _frexpf
        jp      ___fs2fh
