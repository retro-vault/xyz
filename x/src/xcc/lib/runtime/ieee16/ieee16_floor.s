        ; ieee16_floor.s
        .module ieee16_floor
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_floor
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _floorf

        .area   _CODE
_ieee16_floor::
        call    ___fh2fs
        call    _floorf
        jp      ___fs2fh
