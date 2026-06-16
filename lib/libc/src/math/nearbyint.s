        ;; nearbyint.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nearbyint
        .optsdcc -mz80 sdcccall(1)

        .globl  _nearbyint
        .globl  _nearbyintl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  _nearbyintf

        .area   _CODE
_nearbyint::
_nearbyintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        call    _nearbyintf
        call    ___fs2db
        pop     ix
        ret

