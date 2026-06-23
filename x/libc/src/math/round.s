        ;; round.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module round
        .optsdcc -mz80 sdcccall(1)

        .globl  _round
        .globl  _roundl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _roundf

        .area   _CODE
_round::
_roundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _roundf
        call    ___fs2db
        pop     ix
        ret

