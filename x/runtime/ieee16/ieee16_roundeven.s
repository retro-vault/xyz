        ; ieee16_roundeven.s
        .module ieee16_roundeven
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_roundeven
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _roundevenf

        .area   _CODE
_ieee16_roundeven::
        call    ___fh2fs
        call    _roundevenf
        jp      ___fs2fh
