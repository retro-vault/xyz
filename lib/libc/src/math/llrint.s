        ;; llrint.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module llrint
        .optsdcc -mz80 sdcccall(1)

        .globl  _llrint
        .globl  _llrintl
        .globl  __db_load_arg0_fs
        .globl  _llrintf

        .area   _CODE
_llrint::
_llrintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _llrintf               ; 64-bit integer result bypasses fs2db

