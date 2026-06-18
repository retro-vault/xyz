        ;; totalorder.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module totalorder
        .optsdcc -mz80 sdcccall(1)

        .globl  _totalorder
        .globl  _totalorderl
        .globl  _totalorderf
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs

        .area   _CODE
_totalorder::
_totalorderl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _totalorderf
        ; returns int in DE
        ld      sp,ix
        pop     ix
        ret
