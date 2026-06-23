        ;; expm1.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module expm1
        .optsdcc -mz80 sdcccall(1)

        .globl  _expm1
        .globl  _expm1l
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _expm1f

        .area   _CODE
_expm1::
_expm1l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _expm1f
        call    ___fs2db
        pop     ix
        ret

