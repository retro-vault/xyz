        ; ieee16_cbrt.s
        .module ieee16_cbrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_cbrt
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _cbrtf

        .area   _CODE
_ieee16_cbrt::
        call    ___fh2fs
        call    _cbrtf
        jp      ___fs2fh
