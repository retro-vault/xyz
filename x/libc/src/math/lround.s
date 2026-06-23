        ;; lround.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lround
        .optsdcc -mz80 sdcccall(1)

        .globl  _lround
        .globl  _lroundl
        .globl  __db_load_arg0_fs
        .globl  _lroundf

        .area   _CODE
_lround::
_lroundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _lroundf               ; integer result stays in the float ABI

