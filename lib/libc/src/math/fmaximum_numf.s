        ;; fmaximum_numf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaximum_numf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaximum_numf
        .globl  _fmaxf

        .area   _CODE
_fmaximum_numf::
        jp      _fmaxf
