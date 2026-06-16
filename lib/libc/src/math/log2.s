        ;; log2.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module log2
        .optsdcc -mz80 sdcccall(1)

        .globl  _log2
        .globl  _log2l
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _log2f

        .area   _CODE
_log2::
_log2l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log2f
        call    ___fs2db
        pop     ix
        ret

