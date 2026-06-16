        ;; floor.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module floor
        .optsdcc -mz80 sdcccall(1)

        .globl  _floor
        .globl  _floorl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _floorf

        .area   _CODE
_floor::
_floorl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _floorf
        call    ___fs2db
        pop     ix
        ret

