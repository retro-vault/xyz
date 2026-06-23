        ;; libc_exit_kind.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_exit_kind
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_exit_kind

        .area   _DATA
__libc_exit_kind::
        .dw     0

