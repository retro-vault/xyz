        ; ieee16_fromfp.s
        .module ieee16_fromfp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fromfp
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _fromfpf

        .area   _CODE
_ieee16_fromfp::
        call    ___fh2fs
        call    _fromfpf
        jp      ___fs2fh
