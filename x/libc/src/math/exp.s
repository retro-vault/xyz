        ;; exp.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exp
        .optsdcc -mz80 sdcccall(1)

        .globl  _exp
        .globl  _expl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _expf

        .area   _CODE
_exp::
_expl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _expf
        call    ___fs2db
        pop     ix
        ret

