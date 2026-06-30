        ; ieee16_scalbn.s
        .module ieee16_scalbn
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_scalbn
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _scalbnf

        .area   _CODE
_ieee16_scalbn::
        call    ___fh2fs
        call    _scalbnf
        jp      ___fs2fh
