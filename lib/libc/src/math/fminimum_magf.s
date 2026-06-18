        ;; fminimum_magf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fminimum_magf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fminimum_mag_numf
        .globl  _fminimum_magf
        .globl  _fminf

        .area   _CODE
_fminimum_magf::
_fminimum_mag_numf::
        jp      _fminf
