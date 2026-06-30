        ; ieee16_round.s
        .module ieee16_round
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_round
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _roundf

        .area   _CODE
_ieee16_round::
        call    ___fh2fs
        call    _roundf
        jp      ___fs2fh
