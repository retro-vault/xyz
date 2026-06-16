        ;; fmaximumf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaximumf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaximumf

        .area   _CODE
_fmaximumf::
        jp      _fmaxf

