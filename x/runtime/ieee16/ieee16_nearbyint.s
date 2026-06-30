        ; ieee16_nearbyint.s
        .module ieee16_nearbyint
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_nearbyint
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _nearbyintf

        .area   _CODE
_ieee16_nearbyint::
        call    ___fh2fs
        call    _nearbyintf
        jp      ___fs2fh
