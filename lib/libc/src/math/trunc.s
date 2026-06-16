        ;; trunc.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module trunc
        .optsdcc -mz80 sdcccall(1)

        .globl  _trunc
        .globl  _truncl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _truncf

        .area   _CODE
_trunc::
_truncl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _truncf
        call    ___fs2db
        pop     ix
        ret

