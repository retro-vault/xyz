        ;; fminimum_mag.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fminimum_mag
        .optsdcc -mz80 sdcccall(1)

        .globl  _fminimum_mag
        .globl  _fminimum_mag_num
        .globl  _fminimum_mag_numl
        .globl  _fminimum_magl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_fminimum_mag::
_fminimum_magl::
_fminimum_mag_num::
_fminimum_mag_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fminimum_magf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

