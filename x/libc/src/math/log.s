        ;; log.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module log
        .optsdcc -mz80 sdcccall(1)

        .globl  _log
        .globl  _logl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _logf

        .area   _CODE
_log::
_logl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _logf
        call    ___fs2db
        pop     ix
        ret

