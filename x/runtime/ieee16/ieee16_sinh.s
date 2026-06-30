        ; ieee16_sinh.s
        .module ieee16_sinh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_sinh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _sinhf

        .area   _CODE
_ieee16_sinh::
        call    ___fh2fs
        call    _sinhf
        jp      ___fs2fh
