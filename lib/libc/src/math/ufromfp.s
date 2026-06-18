        ;; ufromfp.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ufromfp
        .optsdcc -mz80 sdcccall(1)

        .globl  _ufromfp
        .globl  _ufromfpl
        .globl  _ufromfpx
        .globl  _ufromfpxl
        .globl  _ufromfpf
        .globl  ___fs2db
        .globl  __db_load_arg0_fs

        .area   _CODE
_ufromfp::
_ufromfpl::
_ufromfpx::
_ufromfpxl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _ufromfpf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret
