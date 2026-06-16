        ;; significand.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module significand
        .optsdcc -mz80 sdcccall(1)

        .globl  _significand
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _significandf

        .area   _CODE
_significand::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _significandf
        call    ___fs2db
        pop     ix
        ret
