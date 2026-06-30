        ; ieee16_acos.s
        .module ieee16_acos
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_acos
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _acosf

        .area   _CODE
_ieee16_acos::
        call    ___fh2fs
        call    _acosf
        jp      ___fs2fh
