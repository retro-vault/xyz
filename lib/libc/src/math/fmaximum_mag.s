        ;; fmaximum_mag.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaximum_mag
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaximum_mag
        .globl  _fmaximum_mag_num
        .globl  _fmaximum_mag_numl
        .globl  _fmaximum_magl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_fmaximum_mag::
_fmaximum_magl::
_fmaximum_mag_num::
_fmaximum_mag_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fmaximum_magf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

