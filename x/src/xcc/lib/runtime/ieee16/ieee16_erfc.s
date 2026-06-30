        ; ieee16_erfc.s
        .module ieee16_erfc
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_erfc
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _erfcf

        .area   _CODE
_ieee16_erfc::
        call    ___fh2fs
        call    _erfcf
        jp      ___fs2fh
