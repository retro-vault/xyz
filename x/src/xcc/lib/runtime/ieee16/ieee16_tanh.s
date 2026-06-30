        ; ieee16_tanh.s
        .module ieee16_tanh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_tanh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _tanhf

        .area   _CODE
_ieee16_tanh::
        call    ___fh2fs
        call    _tanhf
        jp      ___fs2fh
