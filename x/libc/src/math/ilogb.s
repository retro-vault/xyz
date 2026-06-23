        ;; ilogb.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ilogb
        .optsdcc -mz80 sdcccall(1)

        .globl  _ilogb
        .globl  _ilogbl
        .globl  __lgd_load_arg0_fs
        .globl  _ilogbf

        .area   _CODE
_ilogb::
_ilogbl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        pop     ix
        jp      _ilogbf

