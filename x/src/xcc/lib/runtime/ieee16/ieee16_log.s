        ; ieee16_log.s
        .module ieee16_log
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_log
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _logf

        .area   _CODE
_ieee16_log::
        call    ___fh2fs
        call    _logf
        jp      ___fs2fh
