        ;; nextup.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nextup
        .optsdcc -mz80 sdcccall(1)

        .globl  _nextup
        .globl  _nextupl
        .globl  _nextupf
        .globl  ___fs2db
        .globl  __db_load_arg0_fs

        .area   _CODE
_nextup::
_nextupl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _nextupf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret
