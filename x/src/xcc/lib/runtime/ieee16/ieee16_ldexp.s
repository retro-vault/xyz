        ; ieee16_ldexp.s
        .module ieee16_ldexp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_ldexp
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _ldexpf

        .area   _CODE
_ieee16_ldexp::
        call    ___fh2fs
        call    _ldexpf
        jp      ___fs2fh
