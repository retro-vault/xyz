        ;; lgd_load_arg1_raw.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lgd_load_arg1_raw
        .optsdcc -mz80 sdcccall(1)

        .globl  __lgd_load_arg1_fs
        .globl  ___db2fs

        .area   _CODE
__lgd_load_arg1_raw:
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
        ret

__lgd_load_arg1_fs::
        call    __lgd_load_arg1_raw
        jp      ___db2fs

