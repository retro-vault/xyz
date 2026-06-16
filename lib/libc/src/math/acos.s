        ;; acos.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module acos
        .optsdcc -mz80 sdcccall(1)

        .globl  _acos
        .globl  _acosl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _acosf

        .area   _CODE
_acos::
_acosl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _acosf
        call    ___fs2db
        pop     ix
        ret

