        ; ieee16_tan.s
        .module ieee16_tan
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_tan
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _tanf

        .area   _CODE
_ieee16_tan::
        call    ___fh2fs
        call    _tanf
        jp      ___fs2fh
