        ;; log1p.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module log1p
        .optsdcc -mz80 sdcccall(1)

        .globl  _log1p
        .globl  _log1pl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _log1pf

        .area   _CODE
_log1p::
_log1pl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log1pf
        call    ___fs2db
        pop     ix
        ret

