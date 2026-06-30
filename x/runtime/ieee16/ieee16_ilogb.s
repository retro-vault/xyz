        ; ieee16_ilogb.s
        .module ieee16_ilogb
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_ilogb
        .globl  ___fh2fs
        .globl  _ilogbf

        .area   _CODE
_ieee16_ilogb::
        call    ___fh2fs
        jp      _ilogbf
