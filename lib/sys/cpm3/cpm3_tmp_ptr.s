        ;; cpm3_tmp_ptr.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_tmp_ptr
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_ptr

        .area   _DATA
__cpm3_tmp_ptr::
        .dw     0
