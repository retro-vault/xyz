        ;; asin.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module asin
        .optsdcc -mz80 sdcccall(1)

        .globl  _asin
        .globl  _asinl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _asinf

        .area   _CODE
_asin::
_asinl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _asinf
        call    ___fs2db
        pop     ix
        ret

