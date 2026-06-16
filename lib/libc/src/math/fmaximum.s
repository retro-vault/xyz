        ;; fmaximum.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmaximum
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaximum
        .globl  _fmaximum_num
        .globl  _fmaximum_numl
        .globl  _fmaximuml
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_fmaximum::
_fmaximuml::
_fmaximum_num::
_fmaximum_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fmaximumf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

