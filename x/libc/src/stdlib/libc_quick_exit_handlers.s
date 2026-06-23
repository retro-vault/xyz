        ;; libc_quick_exit_handlers.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_quick_exit_handlers
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_quick_exit_count
        .globl  __libc_quick_exit_handlers

QUICKEXIT_SLOTS  .equ 8

        .area   _DATA
__libc_quick_exit_handlers::
        .ds     (QUICKEXIT_SLOTS * 2)
__libc_quick_exit_count::
        .dw     0
