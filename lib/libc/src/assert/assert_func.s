        ;; assert_func.s
        ;; Split from assert_fail.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module assert_func
        .optsdcc -mz80 sdcccall(1)

        .globl  __assert_func

        .area   _DATA
__assert_func::
        .dw     0
