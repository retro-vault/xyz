        ; ieee16_rint.s
        .module ieee16_rint
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_rint
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _rintf

        .area   _CODE
_ieee16_rint::
        call    ___fh2fs
        call    _rintf
        jp      ___fs2fh
