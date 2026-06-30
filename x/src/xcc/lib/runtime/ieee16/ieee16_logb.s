        ; ieee16_logb.s
        .module ieee16_logb
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_logb
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _logbf

        .area   _CODE
_ieee16_logb::
        call    ___fh2fs
        call    _logbf
        jp      ___fs2fh
