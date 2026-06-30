        ; ieee16_log10.s
        .module ieee16_log10
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_log10
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _log10f

        .area   _CODE
_ieee16_log10::
        call    ___fh2fs
        call    _log10f
        jp      ___fs2fh
