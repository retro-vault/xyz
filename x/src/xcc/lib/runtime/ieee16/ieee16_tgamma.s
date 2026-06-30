        ; ieee16_tgamma.s
        .module ieee16_tgamma
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_tgamma
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _tgammaf

        .area   _CODE
_ieee16_tgamma::
        call    ___fh2fs
        call    _tgammaf
        jp      ___fs2fh
