        ;; roundeven.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module roundeven
        .optsdcc -mz80 sdcccall(1)

        .globl  _roundeven
        .globl  _roundevenl
        .globl  _roundevenf
        .globl  ___fs2db
        .globl  __db_load_arg0_fs

        .area   _CODE
_roundeven::
_roundevenl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _roundevenf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret
