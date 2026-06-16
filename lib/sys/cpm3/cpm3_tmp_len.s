        ;; cpm3_tmp_len.s
        ;; Split from cpm3_cmp_fpos_size_iy.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_len
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_len

        .area   _DATA
__cpm3_tmp_len::
        .dw     0
