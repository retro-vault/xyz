        ;; fmaximum_magf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaximum_magf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaximum_mag_numf
        .globl  _fmaximum_magf
        .globl  _fmaxf

        .area   _CODE
_fmaximum_magf::
_fmaximum_mag_numf::
        ; mag version basic alias
        jp      _fmaxf
