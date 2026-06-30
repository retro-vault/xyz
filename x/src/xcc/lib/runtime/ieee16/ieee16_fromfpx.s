        ; ieee16_fromfpx.s
        .module ieee16_fromfpx
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fromfpx
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _fromfpxf

        .area   _CODE
_ieee16_fromfpx::
        call    ___fh2fs
        call    _fromfpxf
        jp      ___fs2fh
