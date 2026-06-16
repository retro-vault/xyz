        ;; cos.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cos
        .optsdcc -mz80 sdcccall(1)

        .globl  _cos
        .globl  _cosl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _cosf

        .area   _CODE
_cos::
_cosl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _cosf
        call    ___fs2db
        pop     ix
        ret

