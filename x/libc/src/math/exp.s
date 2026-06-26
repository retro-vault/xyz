        ;; exp.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module exp
        .optsdcc -mz80 sdcccall(1)

        .globl  _exp
        .globl  _expl
        .globl  _expd_core

        .area   _CODE
_exp::
_expl::
        jp      _expd_core
