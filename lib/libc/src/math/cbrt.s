        ;; cbrt.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cbrt
        .optsdcc -mz80 sdcccall(1)

        .globl  _cbrt
        .globl  _cbrtl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _cbrtf

        .area   _CODE
_cbrt::
_cbrtl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _cbrtf
        call    ___fs2db
        pop     ix
        ret

