        ; ieee16_llrint.s
        .module ieee16_llrint
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_llrint
        .globl  ___fh2fs
        .globl  _llrintf

        .area   _CODE
_ieee16_llrint::
        call    ___fh2fs
        jp      _llrintf
