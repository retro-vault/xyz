        ; ieee16_ufromfp.s
        .module ieee16_ufromfp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_ufromfp
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _ufromfpf

        .area   _CODE
_ieee16_ufromfp::
        call    ___fh2fs
        call    _ufromfpf
        jp      ___fs2fh
