        ; ieee16_asin.s
        .module ieee16_asin
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_asin
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _asinf

        .area   _CODE
_ieee16_asin::
        call    ___fh2fs
        call    _asinf
        jp      ___fs2fh
