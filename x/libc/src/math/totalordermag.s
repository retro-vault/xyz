        ;; totalordermag.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module totalordermag
        .optsdcc -mz80 sdcccall(1)

        .globl  _totalordermag
        .globl  _totalordermagl
        .globl  _totalordermagf
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_totalordermag::
_totalordermagl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _totalordermagf
        ld      sp,ix
        pop     ix
        ret
