        ;; assert_file.s
        ;; Split from assert_fail.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module assert_file
        .optsdcc -mz80 sdcccall(1)

        .globl  __assert_file

        .area   _DATA
__assert_file::
        .dw     0

