        ; ieee16_atan.s
        .module ieee16_atan
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_atan
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _atanf

        .area   _CODE
_ieee16_atan::
        call    ___fh2fs
        call    _atanf
        jp      ___fs2fh
