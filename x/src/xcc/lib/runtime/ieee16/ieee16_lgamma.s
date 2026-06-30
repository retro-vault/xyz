        ; ieee16_lgamma.s
        .module ieee16_lgamma
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_lgamma
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _lgammaf

        .area   _CODE
_ieee16_lgamma::
        call    ___fh2fs
        call    _lgammaf
        jp      ___fs2fh
