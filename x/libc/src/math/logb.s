        ;; logb.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module logb
        .optsdcc -mz80 sdcccall(1)

        .globl  _logb
        .globl  _logbl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _logbf

        .area   _CODE
_logb::
_logbl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _logbf
        call    ___fs2db
        pop     ix
        ret

