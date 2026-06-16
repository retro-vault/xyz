        ;; db_load_arg0_fs.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module db_load_arg0_fs
        .optsdcc -mz80 sdcccall(1)

        .globl  __db_load_arg0_fs
        .globl  ___db2fs

        .area   _CODE
__db_load_arg0_fs::
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        exx
        ld      a,8(ix)
        ld      e,a
        ld      a,9(ix)
        ld      d,a
        ld      a,10(ix)
        ld      l,a
        ld      a,11(ix)
        ld      h,a
        exx
        jp      ___db2fs

