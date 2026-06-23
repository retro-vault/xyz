        ;; llround.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module llround
        .optsdcc -mz80 sdcccall(1)

        .globl  _llround
        .globl  _llroundl
        .globl  __db_load_arg0_fs
        .globl  _llroundf

        .area   _CODE
_llround::
_llroundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _llroundf              ; 64-bit integer result bypasses fs2db

