        ;; fminimum.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fminimum
        .optsdcc -mz80 sdcccall(1)

        .globl  _fminimum
        .globl  _fminimum_num
        .globl  _fminimum_numl
        .globl  _fminimuml
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_fminimum::
_fminimuml::
_fminimum_num::
_fminimum_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fminimumf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

