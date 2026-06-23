        ;; sin.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sin
        .optsdcc -mz80 sdcccall(1)

        .globl  _sin
        .globl  _sinl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _sinf

        .area   _CODE
_sin::
_sinl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _sinf
        call    ___fs2db
        pop     ix
        ret

