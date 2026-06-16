        ;; assert_expr.s
        ;; Split from assert_fail.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module assert_expr
        .optsdcc -mz80 sdcccall(1)

        .globl  __assert_expr

        .area   _DATA
__assert_expr::
        .dw     0

