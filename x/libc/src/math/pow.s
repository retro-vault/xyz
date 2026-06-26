        ;; pow.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module pow
        .optsdcc -mz80 sdcccall(1)

        .globl  _pow
        .globl  _powl
        .globl  _powd_core

        .area   _CODE
_pow::
_powl::
        jp      _powd_core
