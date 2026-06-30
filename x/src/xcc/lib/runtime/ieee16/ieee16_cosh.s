        ; ieee16_cosh.s
        .module ieee16_cosh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_cosh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _coshf

        .area   _CODE
_ieee16_cosh::
        call    ___fh2fs
        call    _coshf
        jp      ___fs2fh
