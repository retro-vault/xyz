        ;; assert_line.s
        ;; Split from assert_fail.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module assert_line
        .optsdcc -mz80 sdcccall(1)

        .globl  __assert_line

        .area   _DATA
__assert_line::
        .dw     0

