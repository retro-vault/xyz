        ;; fminimum_numf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fminimum_numf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fminimum_numf

        .area   _CODE
_fminimum_numf::
        jp      _fminf

