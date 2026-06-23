        ;; exp2.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exp2
        .optsdcc -mz80 sdcccall(1)

        .globl  _exp2
        .globl  _exp2l
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _exp2f

        .area   _CODE
_exp2::
_exp2l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _exp2f
        call    ___fs2db
        pop     ix
        ret

