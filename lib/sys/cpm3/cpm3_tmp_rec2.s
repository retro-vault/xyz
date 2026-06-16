        ;; cpm3_tmp_rec2.s
        ;; Split from sys_cpm3_lseek.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_rec2
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_rec2

        .area   _DATA
__cpm3_tmp_rec2::
        .db     0, 0, 0, 0

