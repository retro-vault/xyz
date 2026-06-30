        ; ieee16_exp.s
        .module ieee16_exp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_exp
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _expf

        .area   _CODE
_ieee16_exp::
        call    ___fh2fs
        call    _expf
        jp      ___fs2fh
