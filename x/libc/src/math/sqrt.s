        ;; sqrt.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sqrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _sqrt
        .globl  _sqrtl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _sqrtf

        .area   _CODE
_sqrt::
_sqrtl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _sqrtf
        call    ___fs2db
        pop     ix
        ret

