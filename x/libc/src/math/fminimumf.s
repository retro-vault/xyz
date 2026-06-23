        ;; fminimumf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fminimumf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fminimumf
        .globl  _fminf

        .area   _CODE
_fminimumf::
        jp      _fminf
