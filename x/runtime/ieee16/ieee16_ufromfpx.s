        ; ieee16_ufromfpx.s
        .module ieee16_ufromfpx
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_ufromfpx
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _ufromfpxf

        .area   _CODE
_ieee16_ufromfpx::
        call    ___fh2fs
        call    _ufromfpxf
        jp      ___fs2fh
