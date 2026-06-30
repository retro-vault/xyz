        ; ieee16_sin.s
        .module ieee16_sin
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_sin
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _sinf

        .area   _CODE
_ieee16_sin::
        call    ___fh2fs
        call    _sinf
        jp      ___fs2fh
