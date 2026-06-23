        ;; db_load_arg1_fs.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module db_load_arg1_fs
        .optsdcc -mz80 sdcccall(1)

        .globl  __db_load_arg1_fs
        .globl  ___db2fs

        .area   _CODE
__db_load_arg1_fs::
        ld      a,12(ix)
        ld      e,a
        ld      a,13(ix)
        ld      d,a
        ld      a,14(ix)
        ld      l,a
        ld      a,15(ix)
        ld      h,a
        exx
        ld      a,16(ix)
        ld      e,a
        ld      a,17(ix)
        ld      d,a
        ld      a,18(ix)
        ld      l,a
        ld      a,19(ix)
        ld      h,a
        exx
        jp      ___db2fs

