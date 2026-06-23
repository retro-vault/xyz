        ;; lgd_load_arg0_fs.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module lgd_load_arg0_fs
        .optsdcc -mz80 sdcccall(1)

        .globl  __lgd_load_arg0_fs
        .globl  ___db2fs
        .globl  __lgd_load_arg0_raw

        .area   _CODE
__lgd_load_arg0_fs::
        call    __lgd_load_arg0_raw
        jp      ___db2fs

