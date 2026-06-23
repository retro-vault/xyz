        ;; ceil.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ceil
        .optsdcc -mz80 sdcccall(1)

        .globl  _ceil
        .globl  _ceill
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _ceilf

        .area   _CODE
_ceil::
_ceill::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _ceilf
        call    ___fs2db
        pop     ix
        ret

