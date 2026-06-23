        ;; atan.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module atan
        .optsdcc -mz80 sdcccall(1)

        .globl  _atan
        .globl  _atanl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _atanf

        .area   _CODE
_atan::
_atanl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _atanf
        call    ___fs2db
        pop     ix
        ret

