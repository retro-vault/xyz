        ;; cpm3_fill_spaces.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).




        .module cpm3_fill_spaces
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_tmp_fcb

FCB_SIZE        .equ 36

        .area   _DATA
__cpm3_tmp_fcb::
        .ds     FCB_SIZE
