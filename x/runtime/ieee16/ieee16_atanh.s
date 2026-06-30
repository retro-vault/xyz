        ; ieee16_atanh.s
        .module ieee16_atanh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_atanh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _atanhf

        .area   _CODE
_ieee16_atanh::
        call    ___fh2fs
        call    _atanhf
        jp      ___fs2fh
