        ; ieee16_log2.s
        .module ieee16_log2
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_log2
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _log2f

        .area   _CODE
_ieee16_log2::
        call    ___fh2fs
        call    _log2f
        jp      ___fs2fh
