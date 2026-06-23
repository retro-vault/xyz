        ;; log10.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module log10
        .optsdcc -mz80 sdcccall(1)

        .globl  _log10
        .globl  _log10l
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _log10f

        .area   _CODE
_log10::
_log10l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log10f
        call    ___fs2db
        pop     ix
        ret

