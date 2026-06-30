        ; ieee16_sqrt.s
        .module ieee16_sqrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_sqrt
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _sqrtf

        .area   _CODE
_ieee16_sqrt::
        call    ___fh2fs
        call    _sqrtf
        jp      ___fs2fh
