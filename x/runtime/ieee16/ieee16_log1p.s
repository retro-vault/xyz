        ; ieee16_log1p.s
        .module ieee16_log1p
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_log1p
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _log1pf

        .area   _CODE
_ieee16_log1p::
        call    ___fh2fs
        call    _log1pf
        jp      ___fs2fh
