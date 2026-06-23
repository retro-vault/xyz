        ;; lrint.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lrint
        .optsdcc -mz80 sdcccall(1)

        .globl  _lrint
        .globl  _lrintl
        .globl  __db_load_arg0_fs
        .globl  _lrintf

        .area   _CODE
_lrint::
_lrintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _lrintf                ; integer result stays in the float ABI

