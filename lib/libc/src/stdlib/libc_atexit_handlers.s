        ;; libc_atexit_handlers.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_atexit_handlers
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_atexit_count
        .globl  __libc_atexit_handlers

ATEXIT_SLOTS     .equ 8

        .area   _DATA
__libc_atexit_handlers::
        .ds     (ATEXIT_SLOTS * 2)
__libc_atexit_count::
        .dw     0
