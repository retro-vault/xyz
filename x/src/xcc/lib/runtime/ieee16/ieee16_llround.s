        ; ieee16_llround.s
        .module ieee16_llround
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_llround
        .globl  ___fh2fs
        .globl  _llroundf

        .area   _CODE
_ieee16_llround::
        call    ___fh2fs
        jp      _llroundf
