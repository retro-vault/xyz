        ;; sqrt.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sqrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _sqrt
        .globl  _sqrtl
        .globl  _sqrtd_core

        .area   _CODE
_sqrt::
_sqrtl::
        jp      _sqrtd_core
