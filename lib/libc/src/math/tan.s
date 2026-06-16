        ;; tan.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module tan
        .optsdcc -mz80 sdcccall(1)

        .globl  _tan
        .globl  _tanl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _tanf

        .area   _CODE
_tan::
_tanl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _tanf
        call    ___fs2db
        pop     ix
        ret

