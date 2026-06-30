        ; ieee16_scalbln.s
        .module ieee16_scalbln
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_scalbln
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _scalblnf

        .area   _CODE
_ieee16_scalbln::
        call    ___fh2fs
        call    _scalblnf
        jp      ___fs2fh
