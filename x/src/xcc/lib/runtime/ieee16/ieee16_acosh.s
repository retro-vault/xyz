        ; ieee16_acosh.s
        .module ieee16_acosh
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_acosh
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _acoshf

        .area   _CODE
_ieee16_acosh::
        call    ___fh2fs
        call    _acoshf
        jp      ___fs2fh
